#include "packages.h"
#include "common/io.h"
#include "common/parsing.h"
#include "common/processing.h"
#include "common/properties.h"
#include "common/settings.h"
#include "detection/os/os.h"

#include <dirent.h>

static uint32_t getNumElementsImpl(const char* dirname, unsigned char type)
{
    DIR* dirp = opendir(dirname);
    if(dirp == NULL)
        return 0;

    uint32_t num_elements = 0;

    struct dirent *entry;
    while((entry = readdir(dirp)) != NULL) {
        if(entry->d_type == type)
            ++num_elements;
    }

    if(type == DT_DIR && num_elements >= 2)
        num_elements -= 2; // accounting for . and ..

    closedir(dirp);

    return num_elements;
}

static uint32_t getNumElements(FFstrbuf* baseDir, const char* dirname, unsigned char type)
{
    uint32_t baseDirLength = baseDir->length;
    ffStrbufAppendS(baseDir, dirname);
    uint32_t num_elements = getNumElementsImpl(baseDir->chars, type);
    ffStrbufSubstrBefore(baseDir, baseDirLength);
    return num_elements;
}

static uint32_t getNumStringsImpl(const char* filename, const char* needle)
{
    FILE* file = fopen(filename, "r");
    if(file == NULL)
        return 0;

    uint32_t count = 0;

    char* line = NULL;
    size_t len = 0;

    while(getline(&line, &len, file) != EOF)
    {
        if(strstr(line, needle) != NULL)
            ++count;
    }

    if(line != NULL)
        free(line);

    fclose(file);

    return count;
}

static uint32_t getNumStrings(FFstrbuf* baseDir, const char* filename, const char* needle)
{
    uint32_t baseDirLength = baseDir->length;
    ffStrbufAppendS(baseDir, filename);
    uint32_t num_elements = getNumStringsImpl(baseDir->chars, needle);
    ffStrbufSubstrBefore(baseDir, baseDirLength);
    return num_elements;
}

static uint32_t getSQLite3Int(const FFinstance* instance, FFstrbuf* baseDir, const char* dbPath, const char* query)
{
    uint32_t baseDirLength = baseDir->length;
    ffStrbufAppendS(baseDir, dbPath);
    uint32_t num_elements = (uint32_t) ffSettingsGetSQLite3Int(instance, baseDir->chars, query);
    ffStrbufSubstrBefore(baseDir, baseDirLength);
    return num_elements;
}

static uint32_t countFilesRecursiveImpl(FFstrbuf* baseDirPath, const char* filename)
{
    uint32_t baseDirPathLength = baseDirPath->length;

    ffStrbufAppendC(baseDirPath, '/');
    ffStrbufAppendS(baseDirPath, filename);
    bool exists = ffFileExists(baseDirPath->chars, S_IFREG);
    ffStrbufSubstrBefore(baseDirPath, baseDirPathLength);
    if(exists)
        return 1;

    DIR* dirp = opendir(baseDirPath->chars);
    if(dirp == NULL)
        return 0;

    ffStrbufAppendC(baseDirPath, '/');
    baseDirPathLength = baseDirPath->length;

    uint32_t sum = 0;

    struct dirent *entry;
    while((entry = readdir(dirp)) != NULL) {
        // According to the PMS, neither category nor package name can begin with '.', so no need to check for . or .. specifically
        if(entry->d_type != DT_DIR || entry->d_name[0] == '.')
            continue;

        ffStrbufAppendS(baseDirPath, entry->d_name);
        sum += countFilesRecursiveImpl(baseDirPath, filename);
        ffStrbufSubstrBefore(baseDirPath, baseDirPathLength);
    }

    closedir(dirp);
    return sum;
}

static uint32_t countFilesRecursive(FFstrbuf* baseDir, const char* dirname, const char* filename)
{
    uint32_t baseDirLength = baseDir->length;
    ffStrbufAppendS(baseDir, dirname);
    uint32_t sum = countFilesRecursiveImpl(baseDir, filename);
    ffStrbufSubstrBefore(baseDir, baseDirLength);
    return sum;
}

static uint32_t getNixPackagesImpl(char* path)
{
    //Nix detection is kinda slow, so we only do it if the dir exists
    if(!ffFileExists(path, S_IFDIR))
        return 0;

    FFstrbuf output;
    ffStrbufInitA(&output, 128);

    //https://github.com/LinusDierheimer/fastfetch/issues/195#issuecomment-1191748222
    FFstrbuf command;
    ffStrbufInitA(&command, 255);
    ffStrbufAppendS(&command, "for x in $(nix-store --query --requisites ");
    ffStrbufAppendS(&command, path);
    ffStrbufAppendS(&command, "); do if [ -d $x ]; then echo $x ; fi ; done | cut -d- -f2- | egrep '([0-9]{1,}\\.)+[0-9]{1,}' | egrep -v '\\-doc$|\\-man$|\\-info$|\\-dev$|\\-bin$|^nixos-system-nixos-' | uniq | wc -l");

    ffProcessAppendStdOut(&output, (char* const[]) {
        "sh",
        "-c",
        command.chars,
        NULL
    });

    int result = (int) strtol(output.chars, NULL, 10);

    ffStrbufDestroy(&command);
    ffStrbufDestroy(&output);

    return (uint32_t) result;
}

static uint32_t getNixPackages(FFstrbuf* baseDir, const char* dirname)
{
    uint32_t baseDirLength = baseDir->length;
    ffStrbufAppendS(baseDir, dirname);
    uint32_t num_elements = getNixPackagesImpl(baseDir->chars);
    ffStrbufSubstrBefore(baseDir, baseDirLength);
    return num_elements;
}

static uint32_t getXBPSImpl(FFstrbuf* baseDir)
{
    DIR* dir = opendir(baseDir->chars);
    if(dir == NULL)
        return 0;

    uint32_t result = 0;

    struct dirent *entry;
    while((entry = readdir(dir)) != NULL)
    {
        if(entry->d_type != DT_REG || strncasecmp(entry->d_name, "pkgdb-", 6) != 0)
            continue;

        ffStrbufAppendC(baseDir, '/');
        ffStrbufAppendS(baseDir, entry->d_name);
        result = getNumStringsImpl(baseDir->chars, "<string>installed</string>");
        break;
    }

    closedir(dir);
    return result;
}

static uint32_t getXBPS(FFstrbuf* baseDir, const char* dirname)
{
    uint32_t baseDirLength = baseDir->length;
    ffStrbufAppendS(baseDir, dirname);
    uint32_t result = getXBPSImpl(baseDir);
    ffStrbufSubstrBefore(baseDir, baseDirLength);
    return result;
}

static uint32_t getSnap(FFstrbuf* baseDir)
{
    uint32_t result = getNumElements(baseDir, "/snap", DT_DIR);

    //Accounting for the /snap/bin folder
    return result > 0 ? result - 1 : 0;
}

#ifdef FF_HAVE_RPM
#include "common/library.h"
#include <rpm/rpmlib.h>
#include <rpm/rpmts.h>
#include <rpm/rpmdb.h>
#include <rpm/rpmlog.h>

static uint32_t getRpmFromLibrpm(const FFinstance* instance)
{
    FF_LIBRARY_LOAD(rpm, &instance->config.librpm, 0, "librpm" FF_LIBRARY_EXTENSION, 12)
    FF_LIBRARY_LOAD_SYMBOL(rpm, rpmReadConfigFiles, 0)
    FF_LIBRARY_LOAD_SYMBOL(rpm, rpmtsCreate, 0)
    FF_LIBRARY_LOAD_SYMBOL(rpm, rpmtsInitIterator, 0)
    FF_LIBRARY_LOAD_SYMBOL(rpm, rpmdbGetIteratorCount, 0)
    FF_LIBRARY_LOAD_SYMBOL(rpm, rpmdbFreeIterator, 0)
    FF_LIBRARY_LOAD_SYMBOL(rpm, rpmtsFree, 0)
    FF_LIBRARY_LOAD_SYMBOL(rpm, rpmlogSetMask, 0)

    // Don't print any error messages
    ffrpmlogSetMask(RPMLOG_MASK(RPMLOG_EMERG));

    if(ffrpmReadConfigFiles(NULL, NULL) != 0)
    {
        dlclose(rpm);
        return 0;
    }

    rpmts ts = ffrpmtsCreate();
    if(ts == NULL)
    {
        dlclose(rpm);
        return 0;
    }

    rpmdbMatchIterator mi = ffrpmtsInitIterator(ts, RPMDBI_LABEL, NULL, 0);
    if(mi == NULL)
    {
        ffrpmtsFree(ts);
        dlclose(rpm);
        return 0;
    }

    int count = ffrpmdbGetIteratorCount(mi);

    ffrpmdbFreeIterator(mi);
    ffrpmtsFree(ts);
    dlclose(rpm);

    return count > 0 ? (uint32_t) count : 0;
}

#endif //FF_HAVE_RPM

static void getPackageCounts(const FFinstance* instance, FFstrbuf* baseDir, FFPackagesResult* packageCounts)
{
    packageCounts->apk += getNumStrings(baseDir, "/lib/apk/db/installed", "C:Q");
    packageCounts->dpkg += getNumStrings(baseDir, "/var/lib/dpkg/status", "Status: ");
    packageCounts->emerge += countFilesRecursive(baseDir, "/var/db/pkg", "SIZE");
    packageCounts->eopkg += getNumElements(baseDir, "/var/lib/eopkg/package", DT_DIR);
    packageCounts->flatpak += getNumElements(baseDir, "/var/lib/flatpak/app", DT_DIR);
    packageCounts->nixDefault += getNixPackages(baseDir, "/nix/var/nix/profiles/default");
    packageCounts->nixSystem += getNixPackages(baseDir, "/run/current-system");
    packageCounts->pacman += getNumElements(baseDir, "/var/lib/pacman/local", DT_DIR);
    packageCounts->pkg += getSQLite3Int(instance, baseDir, "/var/db/pkg/local.sqlite", "SELECT count(id) FROM packages");
    packageCounts->rpm += getSQLite3Int(instance, baseDir, "/var/lib/rpm/rpmdb.sqlite", "SELECT count(blob) FROM Packages");
    packageCounts->snap += getSnap(baseDir);
    packageCounts->xbps += getXBPS(baseDir, "/var/db/xbps");
}

static void getPackageCountsRegular(const FFinstance* instance, FFstrbuf* baseDir, FFPackagesResult* packageCounts)
{
    getPackageCounts(instance, baseDir, packageCounts);

    uint32_t baseDirLength = baseDir->length;
    ffStrbufAppendS(baseDir, FASTFETCH_TARGET_DIR_ETC"/pacman-mirrors.conf");
    if(ffParsePropFile(baseDir->chars, "Branch =", &packageCounts->pacmanBranch) && packageCounts->pacmanBranch.length == 0)
        ffStrbufAppendS(&packageCounts->pacmanBranch, "stable");
    ffStrbufSubstrBefore(baseDir, baseDirLength);
}

static void getPackageCountsBedrock(const FFinstance* instance, FFstrbuf* baseDir, FFPackagesResult* packageCounts)
{
    uint32_t baseDirLength = baseDir->length;

    ffStrbufAppendS(baseDir, "/bedrock/strata");

    DIR* dir = opendir(baseDir->chars);
    if(dir == NULL)
    {
        ffStrbufSubstrBefore(baseDir, baseDirLength);
        return;
    }

    ffStrbufAppendC(baseDir, '/');
    uint32_t baseDirLength2 = baseDir->length;

    struct dirent* entry;
    while((entry = readdir(dir)) != NULL)
    {
        if(entry->d_type != DT_DIR)
            continue;
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        ffStrbufAppendS(baseDir, entry->d_name);
        getPackageCounts(instance, baseDir, packageCounts);
        ffStrbufSubstrBefore(baseDir, baseDirLength2);
    }

    closedir(dir);
    ffStrbufSubstrBefore(baseDir, baseDirLength);
}

void ffDetectPackagesImpl(const FFinstance* instance, FFPackagesResult* result)
{
    FFstrbuf baseDir;
    ffStrbufInitA(&baseDir, 512);
    ffStrbufAppendS(&baseDir, FASTFETCH_TARGET_DIR_ROOT);

    if(ffStrbufIgnCaseCompS(&(ffDetectOS(instance)->id), "bedrock") == 0)
        getPackageCountsBedrock(instance, &baseDir, result);
    else
        getPackageCountsRegular(instance, &baseDir, result);

    // If SQL failed, we can still try with librpm.
    // This is needed on openSUSE, which seems to use a proprietary database file
    // This method doesn't work on bedrock, so we do it here.
    #ifdef FF_HAVE_RPM
        if(result->rpm == 0)
            result->rpm = getRpmFromLibrpm(instance);
    #endif

    ffStrbufSet(&baseDir, &instance->state.platform.homeDir);
    result->nixUser = getNixPackages(&baseDir, "/.nix-profile");

    ffStrbufDestroy(&baseDir);
}