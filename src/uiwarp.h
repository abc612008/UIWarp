#ifndef _UI_WARP_H
#define _UI_WARP_H
#include <pugixml.hpp>
#include <string>
#include <stack>
#include <memory>
#include <utility>
#include <cassert>
#include <sstream>
#include <vector>
namespace UIWarp
{
    using namespace pugi;
    enum class Unit {px, cell, percent};
    class Layout;
    int resolve(std::string length, const Layout& parent, bool is_x);
    namespace
    {
        const std::string Type_Grid = "grid", Type_Table = "table", Type_Control = "control";
        inline std::vector<std::string> split(const std::string &s, char delim)
        {
            std::vector<std::string> elems;
            std::stringstream ss;
            ss.str(s);
            std::string item;
            while (std::getline(ss, item, delim)) elems.push_back(item);
            return elems;
        }
        template<typename T> T _get_property(const xml_node& xml, std::string name, T default_value = T());
        template<> int _get_property<int>(const xml_node& xml, std::string name, int default_value)
        {return xml.attribute(name.c_str()).as_int(default_value);}
        template<> bool _get_property<bool>(const xml_node& xml, std::string name, bool default_value)
        {return xml.attribute(name.c_str()).as_bool(default_value);}
        template<> double _get_property<double>(const xml_node& xml, std::string name, double default_value)
        {return xml.attribute(name.c_str()).as_double(default_value);}
        template<> float _get_property<float>(const xml_node& xml, std::string name, float default_value)
        {return xml.attribute(name.c_str()).as_float(default_value);}
        template<> std::string _get_property<std::string>(const xml_node& xml, std::string name, std::string default_value)
        {return xml.attribute(name.c_str()).as_string(default_value.c_str());}
        template<> const char* _get_property<const char*>(const xml_node& xml, std::string name, const char*  default_value)
        {return xml.attribute(name.c_str()).as_string(default_value);}
    }
    struct Element
    {
        virtual ~Element() {}
        virtual bool is_control() = 0;
    };
    struct Vec4i
    {
        int x, y, w, h;
        bool operator==(const Vec4i& rhs) const { return x == rhs.x && y == rhs.y; }
    };
    class Control :public Element
    {
    public:
        Control(const xml_node& xml, const Layout& parent);
        Vec4i rect;
        std::string data, id, type;
        bool is_control() override { return true; }

        template<typename T> T get_property(std::string name, T default_value = T()) const
        {return _get_property<T>(m_xml, name, default_value);}
    private:
        xml_node m_xml;
    };
    struct IterInfo;
    class Layout :public Element
    {
    public:
        class Iterator
        {
        public:
            Iterator();
            Iterator(Layout parent, xml_node_iterator& begin, xml_node_iterator& end);
            const Iterator& operator++();
            std::shared_ptr<Element> operator*() const;
            bool operator !=(Iterator i) const;
        private:
            std::shared_ptr<std::stack<IterInfo>> m_iters;
        };
        Layout(const xml_node& xml, Layout& parent)
            :m_layout(xml)
        {
            load(xml);
            auto parent_rect = parent.get_rect();
            m_rect.x = parent_rect.x + resolve(get_property<std::string>("x"), parent, true);
            m_rect.y = parent_rect.y + resolve(get_property<std::string>("y"), parent, false);
            m_rect.w = resolve(get_property<std::string>("width"), parent, true);
            m_rect.h = resolve(get_property<std::string>("height"), parent, false);
        }
        // init only
        Layout(const xml_node& ui_xml, const Vec4i& parent_rect)
            :m_rect(parent_rect)
        {
            load(ui_xml);
        }
        std::shared_ptr<Control> get_control(std::string id);
        void load(const xml_node& ui_xml)
        {
            m_layout = ui_xml;
            m_type = ui_xml.name();
        }
        Iterator begin() { return Iterator({ *this,m_layout.children().begin(), m_layout.children().end() }); }
        Iterator end() { return Iterator(); }

        Vec4i get_rect() const { return m_rect; }
        std::string get_type() const { return m_type; }
        bool is_control() override { return false; }

        void set_rect(Vec4i rect) { m_rect = rect; }

        template<typename T>
        T get_property(std::string name, T default_value=T()) const
        { return _get_property<T>(m_layout, name, default_value);}

    private:
        xml_node m_layout;
        Vec4i m_rect;
        std::string m_type;
    };
    struct IterInfo
    {
        Layout parent;
        xml_node_iterator begin, end;
    };
    std::shared_ptr<Element> Layout::Iterator::operator *() const
    {
        const xml_node& xml = *m_iters->top().begin;
        std::string type = xml.name();
        if (type == Type_Grid || type == Type_Table) // container
            return std::make_shared<Layout>(xml, m_iters->top().parent);
        else // control
            return std::make_shared<Control>(xml, m_iters->top().parent);
    }
    std::shared_ptr<Control> Layout::get_control(std::string id)
    {
        for (auto& child : *this)
        {
            if (child->is_control())
            {
                auto control = std::dynamic_pointer_cast<Control>(child);
                if (control->id == id)
                    return std::dynamic_pointer_cast<Control>(child);
            }
        }
        return nullptr;
    }
    Control::Control(const pugi::xml_node& xml, const Layout& parent)
    {
        auto parent_rect = parent.get_rect();

        m_xml = xml;
        rect.x = parent_rect.x + resolve(get_property<std::string>("x"), parent, true);
        rect.y = parent_rect.y + resolve(get_property<std::string>("y"), parent, false);
        rect.w = resolve(get_property<std::string>("width"), parent, true);
        rect.h = resolve(get_property<std::string>("height"), parent, false);
        data = get_property<std::string>("data");
        id = get_property<std::string>("id");
        type = xml.name();
        assert(type == Type_Grid || type == Type_Table || type == Type_Control);
    }
    const Layout::Iterator& Layout::Iterator::operator++()
    {
        const xml_node& xml = *m_iters->top().begin;
        std::string type = xml.name();

        if (type == Type_Grid || type == Type_Table)   // container
        {
            m_iters->push({ Layout(xml, m_iters->top().parent), xml.children().begin(), xml.children().end()});
        }
        else // control
        {
            m_iters->top().begin++;
            while (m_iters->top().begin == m_iters->top().end)
            {
                m_iters->pop();
                if (!m_iters->empty())
                    m_iters->top().begin++;
                else break;
            }
        }
        return *this;
    }
    bool Layout::Iterator::operator !=(Iterator i) const
    {
        if (m_iters->empty() && i.m_iters->empty()) return false;
        if (i.m_iters->empty()) return true;
        return m_iters->top().begin != i.m_iters->top().end;
    }
    Layout::Iterator::Iterator()
    { m_iters.reset(new std::stack<IterInfo>); }
    Layout::Iterator::Iterator(Layout parent, xml_node_iterator& begin, xml_node_iterator& end)
    { m_iters.reset(new std::stack<IterInfo>); m_iters->push({ parent, begin,end }); }
    inline int resolve(std::string length, const Layout& parent, bool is_x)
    {
        int ret = 0;
        int selected_length = is_x ? parent.get_rect().w : parent.get_rect().h;
        auto parts = split(length, '+');
        for (auto& part : parts)
        {
            Unit unit;
            int num;
            bool neg = part[0] == '-';
            if (neg) part = part.substr(1);
            if (part[part.size() - 2] == 'p'&&part[part.size() - 1] == 'x')
            {
                unit = Unit::px;
                num = std::stoi(part.substr(0, part.size() - 2));
            }
            else if (part[part.size() - 1] == 'c')
            {
                unit = Unit::cell;
                num = std::stoi(part.substr(0, part.size() - 1));
            }
            else if (part[part.size() - 1] == '%')
            {
                unit = Unit::percent;
                num = std::stoi(part.substr(0, part.size() - 1));
            }
            else
            {
                assert(false);
            }
            int length_abs = 0;
            switch (unit)
            {
            case Unit::cell:
                assert(parent.get_type() == Type_Table); // unit cell can be only used in table
                length_abs = static_cast<int>(num*selected_length / static_cast<double>(parent.get_property<int>(is_x ? "width-cell" : "height-cell")));
                break;
            case Unit::px:
                length_abs = num;
                break;
            case Unit::percent:
                length_abs = static_cast<int>(selected_length*(num / 100.0));
                break;
            }
            ret += (neg ? -1 : 1)*length_abs;
        }
        if (ret < 0)ret += selected_length;
        return ret;
    }
}
#endif
