
#include "parse_config.h"
#include "function_pool.h"

#include <array>
#include <regex>
#include <iostream>


///*******************************************************
///  Парсинг файла конфигурации с параметрами графиков   *
///*******************************************************

/*
    # Примерный вид файла конфигурации

    constants # опционально
    {
        pi=3.1414;
        r=1.0;
        R=3.0;
    }

    functions # опционально
    {
        func1(x,y)=x*y+1;
        func2(_x)=sin(func1(_x,_x))*0.4;
    }

    plot
    {
        mesh;  # наличие сетки
        x_levels; # наличие линий уровня по оси z
        colored;
        points
        {
            limit=6;# опционально
            s(0.0,1.5,25);
            t(0.0,1.5,25);
            x=func1(s,t);
            y=func2(s*t);
            z=s+t;
        }
    }

    plot{...}
    ...
*/

using std::regex;
using std::regex_match;
using std::regex_search;
using std::smatch;
using std::sregex_token_iterator;



const std::string str_identifier="[_[:alpha:]][_[:alnum:]]*";
const regex reg_identifier=regex(str_identifier);

const std::string str_float_number="[[:digit:]]+(\\.[[:digit:]]*)?";
//const std::string str_float_number="[[:digit:]]+";
const regex reg_float_number=regex(str_float_number);

const std::string str_sign_float_number="-?[[:digit:]]+(\\.[[:digit:]]*)?";
const regex reg_sign_float_number=regex(str_sign_float_number);


//флаги отображения графика
const string_map_t<int> reserved_words=
{
    {"colored",plot3D_chars_t::colored_surfase_id},
    {"specular",plot3D_chars_t::specular_surface_id},
    {"mesh",plot3D_chars_t::mesh_id},
    {"box",plot3D_chars_t::close_box_id},
    {"xlevels",plot3D_chars_t::x_levels_id},
    {"ylevels",plot3D_chars_t::y_levels_id},
    {"zlevels",plot3D_chars_t::z_levels_id}
};

/// coords_functor_t
// Приводит тройку fp_expression_t
// к интерфейсу вызова функции возвращающей вектор

struct coords_functor_t
{
    const fp_expression_t*   m_coord[3];
    mutable std::vector<float> m_stack[3];
    public:
    point3D_t operator ()(float s,float t,float time)const
    {
        for(int i=0;i<3;++i)
        {
            m_stack[i].assign({s,t,time});
            m_coord[i]->call_stack(m_stack[i]);
        }
        return point3D_t(m_stack[0].back(),
                         m_stack[1].back(),
                         m_stack[2].back());
    }
    point3D_t operator ()(float s,float t)const
    {
        return (*this)(s,t,0.0);
    }

    coords_functor_t(const fp_expression_t* _x,
                     const fp_expression_t* _y,
                     const fp_expression_t* _z)
    {
         m_coord[0]=_x;
         m_coord[1]=_y;
         m_coord[2]=_z;
    }
    //~coords_functor_t(){std::cout<<"coords_functor_t delete\n";}
};

// парсинг блока содержащего объявления глобальных
// констант

//pi=3.1414;
//r=1.0;
//R=3.0;
bool ParseConstantsBlock(str_citerator begin,str_citerator end,
                          CFunctionPool&pool)
{
    regex const_def=regex("("+str_identifier+")=(" +
                              str_sign_float_number +")");
    regex  sep_regex(";");
    sregex_token_iterator sti(begin,end,sep_regex,-1);
    smatch smch;
    for(sregex_token_iterator end_it;sti!=end_it;++sti)
    {
        if(sti->first==sti->second) continue;
        if(!regex_match(sti->first,sti->second,smch,const_def)) return false;
        //std::cout<<"constant: "<<smch[1].str()<<std::endl;
        if(!pool.RegisterConstant(smch[1].str(),pool.GetNumberParser()(smch[2].str())))
            return false;
    }
    return true;
}

// парсинг блока содержащего обьявления пользовательских
// функции
// func_name (var_1, var_2,var_3)
bool ParseFunctionDefine(str_citerator begin,str_citerator end,
                         std::string&name,
                         std::vector<std::string>&vars)
{
    regex func_def=regex("("+str_identifier+")"+"\\(([^)]*)\\)");
    smatch smch;
    if(!regex_match(begin,end,smch,func_def)) return false;
    name=smch.str(1);

    vars.clear();
    regex  sep_regex(",");
    sregex_token_iterator sti(smch[2].first,smch[2].second,sep_regex,-1);
    for(sregex_token_iterator end_it;sti!=end_it;++sti)
    {
        if(!regex_match(sti->first,sti->second,reg_identifier)) return false;
        vars.push_back(std::string(sti->first,sti->second));
    }
    return true;
}

// парсинг блока содержащего определения пользовательских
// функций
// func_name (var_1, var_2,var_3)=var_1+var_3;
// func_name2(...)=....;...;...;;

std::pair<parse_error_t,str_citerator>
ParseFunctionsBlock(str_citerator begin,str_citerator end,CFunctionPool&pool)
{
    //std::cout<<"function block: "<<std::string(begin,end)<<std::endl;
    std::string name;
    std::vector<std::string> vars;
    auto parse_func_def=[&name,&vars,&pool](str_citerator begin,str_citerator end)
    ->parse_error_t
    {
        str_citerator equal_char_=find(begin,end,'=');
        if(equal_char_==end||!ParseFunctionDefine(begin,equal_char_,name,vars))
            return {parse_error_t::unknown_error_id,""};
        return pool.RegisterFunction(name,vars,equal_char_+1,end);
    };

    regex  sep_regex(";");
    sregex_token_iterator sti(begin,end,sep_regex,-1);
    for(sregex_token_iterator end_it;sti!=end_it;++sti)
    {
        if(sti->first==sti->second) continue;
        //std::cout<<"function: "<<std::string(sti->first,sti->second)<<std::endl;
        if(auto pe=parse_func_def(sti->first,sti->second);!pe.is_valid())
            return {pe,sti->first};
    }
    return {parse_error_t(parse_error_t::success_id,""),end};
}


// парсинг блока флаги отображения графика
/*
    mesh;
    xlevels;
    ylevels;
    colored;
*/
bool ParseFlagsBlock (str_citerator begin,str_citerator end,plot3D_chars_t& _chars)
{
    auto flag_reg=regex("([[:alpha:]]+)");

    regex  sep_regex(";");
    sregex_token_iterator sti(begin,end,sep_regex,-1);
    for(sregex_token_iterator end_it;sti!=end_it;++sti)
    {
        if(sti->first==sti->second) continue;
        if(!regex_match(sti->first,sti->second,flag_reg)) return false;
        auto iter=reserved_words.find({sti->first,sti->second});
        if(iter==reserved_words.end()) return false;
        //std::cout<<"flag:"<<std::string(sti->first,sti->second)<<std::endl;
        _chars.m_type|=iter->second;
    }
    return true;
}

/*
    period=34;# опционально
    s(0.0,1.5,25);
    t(0.0,1.5,25);
    x=func1(s,t);
    y=func2(s*t);
    z=s+t;
*/
std::pair<parse_error_t,str_citerator>
ParsePointsBlock(str_citerator begin,str_citerator end,
                 CFunctionPool&pool,
                 animate_chars_t&animate,
                 plot3D_chars_t&_chars)
{
    //std::cout<<"points block:"<<std::string(begin,end)<<std::endl;
    smatch smch;
    animate.m_type=animate_chars_t::no_animate_id;
    regex anim_reg=regex(std::string("^(period|limit)=")+
                         "(" + str_float_number + ");");
    if(regex_search(begin,end,smch,anim_reg))
    {
        animate.m_type=(smch.str(1)=="period")?
                        animate_chars_t::periodical_id :
                        animate_chars_t::finite_id;
        animate.m_time=pool.GetNumberParser()(smch.str(2));
        begin=smch.suffix().first;
        //std::cout<<"anumate\n";
    }

    std::string point_reg="x=([^;]*);y=([^;]*);z=([^;]*);";

    auto range_regex=[](char char_)
    {
        return   std::string(1,char_)+"\\("+
                "("+str_sign_float_number+")"+","+
                "("+str_sign_float_number+")"+","+
                "("+str_float_number+")"+"\\)"+";";
    };

    regex reg_ranges=regex(range_regex('s')+range_regex('t')+point_reg);
    if(!regex_match(begin,end,smch,reg_ranges))
    {
        std::cout<<"fail reg_ranges\n";
        return {parse_error_t(parse_error_t::unknown_error_id,"Error in ranges"),
                              begin};
    }
    auto ston=pool.GetNumberParser();
    _chars.m_s_range={ston(smch.str(1)),ston(smch.str(3))};
    _chars.m_num_s_range=ston(smch.str(5));

    _chars.m_t_range={ston(smch.str(7)),ston(smch.str(9))};
    _chars.m_num_t_range=ston(smch.str(11));

    //std::cout<<"_x: "<<smch.str(13)<<std::endl;
    auto _x=pool.CreateFunction({"s","t","time"},smch[13].first,smch[13].second);
    if(!_x.first.is_valid())  return {_x.first,smch[13].first};

    //std::cout<<"_y: "<<smch.str(14)<<std::endl;
    auto _y=pool.CreateFunction({"s","t","time"},smch[14].first,smch[14].second);
    if(!_y.first.is_valid()) return {_y.first,smch[14].first};

    //std::cout<<"_z: "<<smch.str(15)<<std::endl;
    auto _z=pool.CreateFunction({"s","t","time"},smch[15].first,smch[15].second);
    if(!_z.first.is_valid()) return {_z.first,smch[15].first};

    if(animate.m_type!=animate_chars_t::no_animate_id)
    {
        animate.m_coords_functor=coords_functor_t(_x.second,_y.second,_z.second);
        _chars.m_coords_functor=nullptr;
    }
    else
    {
        animate.m_coords_functor=nullptr;
        _chars.m_coords_functor=coords_functor_t(_x.second,_y.second,_z.second);
    }
    return {_z.first,end};;
}

parse_config_result_t ParseConfig(std::string&file,CFunctionPool&pool)
{
    pool.Clear();
    // удаление комментариев
    using str_iterator=std::string::iterator;
    str_iterator current=file.begin();
    while(current!=file.end())
    {
        str_iterator comm_begin=std::find(current,file.end(),'#');
        current=std::find(comm_begin,file.end(),'\n');
        current=file.erase(comm_begin,current);
    }
    //перенумеровываем строки
    // и удаляем пробельные символы
    std::vector<int> str_number(file.size());
    {
        int j=0;
        int current_number=1;
        for(unsigned int i=0;i<file.size();++i)
        {
            if(file[i]=='\n')
            {
                current_number++;
                continue;
            }
            if(!is_ignore(file[i]))
            {
                file[j]=file[i];
                str_number[j++]=current_number;
            }
        }
        file.resize(j); str_number.resize(j);
    }
    //получить индекс строки по строковому итератору
    auto  get_str_index=[&str_number,&file](str_citerator iter)
    {
        return str_number[iter-file.cbegin()];
    };
    smatch smch;
    // парсинг необязательного constants блока
    str_citerator necess_begin=file.cbegin();
    auto const_reg=regex("^constants\\{([^\\}]*)\\}");
    if(regex_search(file.cbegin(),file.cend(),smch,const_reg))
    {
        //std::cout<<"parse constants block\n";
        if(!ParseConstantsBlock(smch[1].first,smch[1].second,pool))
        {
            return {
                parse_error_t(parse_error_t::unknown_error_id,"Error in constants block"),
                get_str_index(smch[0].first)
            };
        }
        necess_begin=smch[0].second;
    }

    // парсинг необязательного  functions блока
    auto func_reg=regex("^functions\\{([^\\}]*)\\}");
    if(regex_search(necess_begin,file.cend(),smch,func_reg))
    {
        if(auto pfb=ParseFunctionsBlock(smch[1].first,smch[1].second,pool);
        !pfb.first.is_valid())
        {
            return {pfb.first,get_str_index(pfb.second)};
        }
        necess_begin=smch[0].second;
    }

    regex plot_reg=regex("^\\{([^\\{\\}]*)points\\{([^\\{\\}]*)\\}\\}");
    parse_config_result_t parse_result;
    auto plot_parse=[&parse_result,&plot_reg,&pool,get_str_index](str_citerator pl_begin,str_citerator pl_end)
    {
        smatch smch;
        parse_result.m_plots.resize(parse_result.m_plots.size()+1);
        auto &[animate_d,plot_d]=parse_result.m_plots.back();
        if(regex_match(pl_begin,pl_end,smch,plot_reg))
        {
            if(!ParseFlagsBlock(smch[1].first,smch[1].second,plot_d))
            {
                parse_result.m_parse_result=parse_error_t(parse_error_t::unknown_error_id,"Error in flags block");
                parse_result.m_index_error_string=get_str_index(smch[1].first);
                return false;
            }
            if(auto [pe,iter]=ParsePointsBlock(smch[2].first,smch[2].second,pool,animate_d,plot_d);
            !pe.is_valid())
            {
                parse_result.m_parse_result=pe;
                parse_result.m_index_error_string=get_str_index(iter);
                return false;
            }
            return true;
        }
        else
        {
           parse_result.m_parse_result=parse_error_t(parse_error_t::unknown_error_id,"Error in plot block");
           parse_result.m_index_error_string=get_str_index(pl_begin);
           return false;
        }
    };

    regex  sep_regex("plot");
    sregex_token_iterator sti(smch.suffix().first,file.end(),sep_regex,-1);
    for(sregex_token_iterator end_it;sti!=end_it;++sti)
    {
        if(sti->first==sti->second) continue;
        if(!plot_parse(sti->first,sti->second))
        {
            return parse_result;
        }
    }
    parse_result.m_parse_result={parse_error_t::success_id,""};
    return parse_result;
}

std::string ErrorReport(const parse_config_result_t& pcr)
{
    if(pcr.is_valid()) return "";
    auto iter=parse_error_map.find(pcr.m_parse_result.type());
    assert(iter!=parse_error_map.end());
    return std::string("Error:")+iter->second+","+
    pcr.m_parse_result.detail()+"\nLine:"+
    std::to_string(pcr.m_index_error_string)+"\n";

}

