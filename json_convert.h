#ifndef _json_convert_
#define _json_convert_

#include "Expression/data_pool.h"

std::string FromJson(const std::string&,CDataPool&);
void ToJson(std::string&,const CDataPool&);


#endif // _json_convert_
