#ifndef __STRING_UTILS_H__
#define __STRING_UTILS_H__
#include <vector>
#include <tuple>
#include <WString.h>
using namespace std;

class StringUtils{
public:
    static vector<String> split(String source, String splitBy, bool doNotSplitTextInsideQuotations = true, bool removeQuotationMarkers = true);
    static vector<String> split(String source, vector<String> splitBys, bool doNotSplitTextInsideQuotations = true, bool removeQuotationMarkers = true);
    static String replace(String source, vector<tuple<String, String>> replacesAndBys);
    static String replace(String source, vector<String> by, String marker = "?", bool use_TheArgBy_Circularly = false);
    static String getOnly(String source, String allowedChars);
    static bool IsNumber(String source, double min = INT32_MIN , double max = INT32_MAX);
    static bool IsInteger(String source, int min = INT32_MIN , int max = INT32_MAX);
};

#define S String
#define SR StringUtils::replace
#define SRA(s, args) StringUtils::replace(s, { args })


#endif
