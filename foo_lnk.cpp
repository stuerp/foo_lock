
/** $VER: foo_lnk.cpp (2023.06.18) **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4710 4711 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include <sdk/foobar2000-lite.h>
#include <sdk/componentversion.h>
#include <sdk/input_file_type.h>
#include <sdk/link_resolver.h>
#include <sdk/initquit.h>

#include <pfc/pfc-lite.h>

#include "resource.h"

#pragma hdrstop

#include "shlobj.h"

#define COMPONENT_NAME    "Shell Link Resolver"
#define COMPONENT_VERSION "1.3.2"
#define COMPONENT_ABOUT   "Resolves shell links (.lnk)\n" \
                          "\n" \
                          "written by Holger Stenger"

namespace
{
    #pragma warning(disable: 4265 4625 4626 5026 5027 26433 26436 26455)
    DECLARE_COMPONENT_VERSION
    (
        COMPONENT_NAME,
        COMPONENT_VERSION,
        COMPONENT_ABOUT
    );

    VALIDATE_COMPONENT_FILENAME("foo_lnk.dll");

    DECLARE_FILE_TYPE("Shell Links", "*.LNK");
}

void uResolveLink(const char * linkfile, pfc::string_base & path, HWND hwnd = 0);

class LinkResolver : public link_resolver
{
public:
    //! Tests whether specified file is supported by this link_resolver service.
    //! @param p_path Path of file being queried.
    //! @param p_extension Extension of file being queried. This is provided for performance reasons, path already includes it.
    virtual bool is_our_path(const char * filePath, const char * fileExtension)
    {
        UNREFERENCED_PARAMETER(filePath);

        return (pfc::stricmp_ascii(fileExtension, "lnk") == 0);
    }

    //! Resolves a link file. Before this is called, path must be accepted by is_our_path().
    //! @param p_filehint Optional file interface to use. If null/empty, implementation should open file by itself.
    //! @param p_path Path of link file to resolve.
    //! @param p_out Receives path the link is pointing to.
    //! @param p_abort abort_callback object signaling user aborting the operation.
    virtual void resolve(service_ptr_t<file> file, const char * linkFilePath, pfc::string_base & targetFilePath, abort_callback & abortHandler)
    {
        UNREFERENCED_PARAMETER(abortHandler);

        if (::stricmp_utf8_partial("file://", linkFilePath, 7) != 0)
            throw exception_io_data(pfc::stringLite("shortcut is not on local filesystem:\n") + linkFilePath);

        const char * LinkFilePath = linkFilePath + 7;

        uResolveLink(LinkFilePath, targetFilePath);
    }
};

void uResolveLink(const char * linkFilePath, pfc::string_base & filePath, HWND hWnd)
{
    filePath.reset();

    const DWORD Flags = (DWORD)(hWnd ? 0 : SLR_NO_UI);

    // Get a pointer to the IShellLink interface (Unicode version). 
    pfc::com_ptr_t<IShellLinkW> pslw;

    pfc::string8 ShortPath;

    HRESULT hResult = ::CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (LPVOID *) pslw.receive_ptr());

    if (SUCCEEDED(hResult))
    {
        pfc::com_ptr_t<IPersistFile> pf;

        // Get a pointer to the IPersistFile interface. 
        hResult = pslw->QueryInterface(IID_IPersistFile, (void **) pf.receive_ptr());

        if (FAILED(hResult))
            throw exception_win32((DWORD)hResult);

        // Load the shortcut. 
        hResult = pf->Load(pfc::stringcvt::string_wide_from_utf8(linkFilePath), STGM_READ);

        if (FAILED(hResult))
            throw exception_win32((DWORD)hResult);

        // Resolve the link.
        hResult = pslw->Resolve(hWnd, Flags);

        if (FAILED(hResult))
            throw exception_win32((DWORD)hResult);

        WCHAR ShortPathArray[MAX_PATH];
        WIN32_FIND_DATAW wfd = { 0 };

        // Get the path to the link target.
        hResult = pslw->GetPath(ShortPathArray, MAX_PATH, &wfd, 0);

        if (FAILED(hResult))
            throw exception_win32((DWORD)hResult);

        ShortPath.set_string(pfc::stringcvt::string_utf8_from_wide(ShortPathArray));
    }
    else
    {
        // Get a pointer to the IShellLink interface (ANSI version).
        pfc::com_ptr_t<IShellLinkA> psla;

        hResult = ::CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkA, (LPVOID *) &psla);

        if (FAILED(hResult))
            throw exception_win32((DWORD)hResult);

        pfc::com_ptr_t<IPersistFile> pf;

        // Get a pointer to the IPersistFile interface. 
        hResult = psla->QueryInterface(IID_IPersistFile, (void **) pf.receive_ptr());

        if (FAILED(hResult))
            throw exception_win32((DWORD)hResult);

        // Load the shortcut. 
        hResult = pf->Load(pfc::stringcvt::string_wide_from_utf8(linkFilePath), STGM_READ);

        if (FAILED(hResult))
            throw exception_win32((DWORD)hResult);

        // Resolve the link.
        hResult = psla->Resolve(hWnd, Flags);

        if (FAILED(hResult))
            throw exception_win32((DWORD)hResult);

        char ShortPathArray[MAX_PATH];
        WIN32_FIND_DATAA wfd = { 0 };

        // Get the path to the link target.
        hResult = psla->GetPath(ShortPathArray, MAX_PATH, &wfd, 0);

        if (FAILED(hResult))
            throw exception_win32((DWORD)hResult);

        ShortPath.set_string(pfc::stringcvt::string_utf8_from_ansi(ShortPathArray));
    }

    if (uGetLongPathName(ShortPath, filePath) == 0)
        filePath.set_string(ShortPath, ShortPath.length());
}

static service_factory_single_t<LinkResolver> _LinkResolverFactory;

#pragma warning(disable: 4820)
class initquit_lnk : public initquit
{
public:
    virtual void on_init()
    {
        HRESULT hResult = ::CoInitialize(NULL);

        _Initialized = !!SUCCEEDED(hResult);

        if (FAILED(hResult))
            throw exception_win32((DWORD)hResult);
    }

    virtual void on_quit()
    {
        if (_Initialized)
            ::CoUninitialize();
    }

private:
    bool _Initialized = false;
};

static initquit_factory_t< initquit_lnk > _InitQuitFactory;
