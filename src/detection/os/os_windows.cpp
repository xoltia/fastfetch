extern "C" {
#include "os.h"
#include "util/windows/unicode.h"
}
#include "util/windows/wmi.hpp"

static const char* getOsNameByWmi(FFstrbuf* osName)
{
    FFWmiQuery query(L"SELECT Caption FROM Win32_OperatingSystem");
    if(!query)
        return "Query WMI service failed";

    if(FFWmiRecord record = query.next())
    {
        record.getString(L"Caption", osName);
        ffStrbufTrimRight(osName, ' ');
        return NULL;
    }

    return "No WMI result returned";
}

PWSTR WINAPI BrandingFormatString(PCWSTR format);

static const char* getOsNameByWinbrand(FFstrbuf* osName)
{
    //https://dennisbabkin.com/blog/?t=how-to-tell-the-real-version-of-windows-your-app-is-running-on#ver_string
    FF_LIBRARY_LOAD(winbrand, nullptr, "dlopen winbrand" FF_LIBRARY_EXTENSION " failed", "winbrand" FF_LIBRARY_EXTENSION, 1);
    FF_LIBRARY_LOAD_SYMBOL_MESSAGE(winbrand, BrandingFormatString);

    const wchar_t* rawName = ffBrandingFormatString(L"%WINDOWS_LONG%");
        ffStrbufSetWS(osName, rawName);
        GlobalFree((HGLOBAL)rawName);
        return NULL;
}

extern "C"
void ffDetectOSImpl(FFOSResult* os)
{
    ffStrbufInit(&os->name);
    ffStrbufInit(&os->prettyName);
    ffStrbufInit(&os->id);
    ffStrbufInit(&os->idLike);
    ffStrbufInit(&os->variant);
    ffStrbufInit(&os->variantID);
    ffStrbufInit(&os->version);
    ffStrbufInit(&os->versionID);
    ffStrbufInit(&os->codename);
    ffStrbufInit(&os->buildID);

    if(getOsNameByWinbrand(&os->variant) && getOsNameByWmi(&os->variant))
        return;

    ffStrbufTrimRight(&os->variant, ' ');

    //WMI returns the "Microsoft" prefix while BrandingFormatString doesn't. Make them consistant.
    if(ffStrbufStartsWithS(&os->variant, "Microsoft "))
        ffStrbufSubstrAfter(&os->variant, strlen("Microsoft ") - 1);

    if(ffStrbufStartsWithS(&os->variant, "Windows "))
    {
        ffStrbufAppendS(&os->name, "Windows");
        ffStrbufAppendS(&os->prettyName, "Windows");

        ffStrbufSubstrAfter(&os->variant, strlen("Windows ") - 1);

        if(ffStrbufStartsWithS(&os->variant, "Server "))
        {
            ffStrbufAppendS(&os->name, " Server");
            ffStrbufAppendS(&os->prettyName, " Server");
            ffStrbufSubstrAfter(&os->variant, strlen(" Server") - 1);
        }

        uint32_t index = ffStrbufFirstIndexC(&os->variant, ' ');
        ffStrbufAppendNS(&os->version, index, os->variant.chars);
        ffStrbufSubstrAfter(&os->variant, index);

        // Windows Server 20xx Rx
        if(ffStrbufEndsWithC(&os->prettyName, 'r'))
        {
            if(os->variant.chars[0] == 'R' &&
            isdigit(os->variant.chars[1]) &&
            (os->variant.chars[2] == '\0' || os->variant.chars[2] == ' '))
            {
                ffStrbufAppendF(&os->version, " R%c", os->variant.chars[1]);
                ffStrbufSubstrAfter(&os->variant, strlen("Rx ") - 1);
            }
        }
    }
    else
    {
        // Unknown Windows name, please report this
        ffStrbufAppend(&os->name, &os->variant);
        ffStrbufClear(&os->variant);
    }

    ffStrbufAppendF(&os->id, "%*s %*s", os->prettyName.length, os->prettyName.chars, os->version.length, os->version.chars);
}
