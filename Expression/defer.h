
#ifndef  _defer_
#define  _defer_

template<class func_t>
class scope_guard_t
{
    bool m_exe=true;
    func_t m_func;
    public:
    explicit scope_guard_t(func_t func):
    m_func(func){}
    scope_guard_t(const scope_guard_t&)=delete;
    scope_guard_t& operator=(const scope_guard_t&)=delete;
    bool& exe(){return m_exe;}
    ~scope_guard_t()
    {
        if(m_exe) m_func();
    }
};

template<class func_t>
scope_guard_t<func_t> make_scope_guard(func_t f)
{return scope_guard_t<func_t>(f);}

#endif

