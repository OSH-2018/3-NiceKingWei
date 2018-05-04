//
// Created by nicekingwei on 18-4-30.
//

#include "global.h"
#include "block_string.h"


// singleton
driver_object driver;


bool skipnode::operator<(const char *str_s2) const {
    if(filename.isnull()) return true;  // null is minimal

    static_string& s1 = *filename;
    int len_s1 = s1.n;
    const char* str_s1 = s1.str();
    for(int i=0;i<len_s1;i++){
        if(str_s1[i]<str_s2[i]) return true;
        else if(str_s1[i]>str_s2[i]) return false;
    }
    return false;
}

bool skipnode::operator==(const char *str_s2) const {
    if(filename.isnull()) return false;  // null is minimal

    static_string& s1 = *filename;
    int len_s1 = s1.n;
    const char* str_s1 = s1.str();
    for(int i=0;i<len_s1;i++){
        if(str_s1[i]!=str_s2[i]) return false;
    }
    return str_s2[len_s1]=='\0';
}

bool skipnode::operator>=(const char *str_s2) const {
    return !((*this)<str_s2);
}
