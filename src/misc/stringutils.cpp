#include "stringutils.h"

vector<String> StringUtils::split(String source, String splitBy, bool doNotSplitTextInsideQuotations, bool removeQuotationMarkers)
{
    if (splitBy == "")
        return {source};

    vector<String> ret;
    char skeepingChar = 0;
    String curr = "";
    for (unsigned int index = 0; index < source.length(); index++)
    {
        char c = source[index];
        if ((doNotSplitTextInsideQuotations) && (c == '"' || c == '\''))
        {
            if (skeepingChar == c)
                skeepingChar = 0;
            else
                skeepingChar = c;

            if (!removeQuotationMarkers)
                curr += c;
        }
        else if (skeepingChar != 0)
        {
            curr += c;
        }
        else
        {
            if (c == splitBy[0] && source.substring(index, index+splitBy.length()) == splitBy)
            {
                ret.push_back(curr);
                curr = "";
                index += splitBy.length()-1;
            }
            else
                curr += c;
        }
    }
    ret.push_back(curr);
    return ret;
}

vector<String> StringUtils::split(String source, vector<String> splitBys, bool doNotSplitTextInsideQuotations, bool removeQuotationMarkers)
{
    String newSplitter = "__NeWSpL__";
    for (auto &c: splitBys)
        source.replace(c, newSplitter);

    return StringUtils::split(source, newSplitter, doNotSplitTextInsideQuotations, removeQuotationMarkers);

}

String StringUtils::replace(String source, vector<tuple<String, String>> replacesAndBys)
{
    for (auto &currTuple: replacesAndBys)
        source.replace(std::get<0>(currTuple), std::get<1>(currTuple));

    return source;
}

String StringUtils::replace(String source, vector<String> by, String marker, bool use_TheArgBy_Circularly)
{
    String ret;
    int pos = (source.indexOf(marker));
    size_t index = 0;
    while (pos > -1 && index < by.size())
    {
        auto currReplacer = by[index];
        ret += source.substring(0, pos) + currReplacer;
        source = source.substring(pos + marker.length());
        pos = source.indexOf(marker);

        index++;
        if (use_TheArgBy_Circularly && index >= by.size())
            index = 0;
    }
    
    ret += source;
    return ret;
}

String StringUtils::getOnly(String source, String allowedChars)
{
    String result = "";
    for (auto &c: source)
        if (allowedChars.indexOf(c) > -1)
            result += c;

    return result;
}

bool StringUtils::IsNumber(String source, double min, double max)
{
    if (source == "")
        return false;

        
        //if (source[0] == '-' || source[0] == '+')
        //    source = source.substring(1);
        
    if (source.length() > 11)
        return false;

    if (getOnly(source, "0123456789-.") != source)
        return false;

    double value = source.toDouble();
    if (value < min || value > max)
        return false;

    return true;
}

bool StringUtils::IsInteger(String source, int min, int max)
{
    if (source == "")
    return false;

    
    //if (source[0] == '-' || source[0] == '+')
    //    source = source.substring(1);
    
    if (source.length() > 11)
        return false;

    if (getOnly(source, "0123456789-") != source)
        return false;

    long long int value = source.toInt();
    if (value < min || value > max)
        return false;

    return true;
}
