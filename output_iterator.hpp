#ifndef _output_iterator_
#define _output_iterator_

namespace ext
{

template <class UnaryFunction>
class fn_output_iterator
{
    typedef fn_output_iterator self;
    public:

    explicit fn_output_iterator() {}

    explicit fn_output_iterator(const UnaryFunction& f)
      : m_f(f) {}

    struct output_proxy
    {
        output_proxy(UnaryFunction& f) : m_f(f) { }
        template <class T> output_proxy& operator=(const T& value)
        {
            m_f(value);
            return *this;
        }
        UnaryFunction& m_f;
    };
    output_proxy operator*() { return output_proxy(m_f); }
    self& operator++() { return *this; }
    self& operator++(int) { return *this; }
    private:
    UnaryFunction m_f;
};

} // ext


#endif
