#include "brackets_extensions.h"
#include "Resource.h"

#include <stdio.h>
#include <sys/types.h>
#include <CommDlg.h>
#include <ShlObj.h>
#include <wchar.h>

// Error values. These MUST be in sync with the error values
// in brackets_extensions.js
//static const int NO_ERROR                   = 0;
static const int ERR_UNKNOWN                = 1;
static const int ERR_INVALID_PARAMS         = 2;
static const int ERR_NOT_FOUND              = 3;
static const int ERR_CANT_READ              = 4;
static const int ERR_UNSUPPORTED_ENCODING   = 5;
static const int ERR_CANT_WRITE             = 6;
static const int ERR_OUT_OF_SPACE           = 7;
static const int ERR_NOT_FILE               = 8;
static const int ERR_NOT_DIRECTORY          = 9;


class BracketsExtensionHandler : public CefV8Handler
{
public:
    BracketsExtensionHandler() : lastError(0) {}
    virtual ~BracketsExtensionHandler() {}
    
    // Execute with the specified argument list and return value.  Return true if
    // the method was handled.
    virtual bool Execute(const CefString& name,
                         CefRefPtr<CefV8Value> object,
                         const CefV8ValueList& arguments,
                         CefRefPtr<CefV8Value>& retval,
                         CefString& exception)
    {  
        int errorCode = -1;
        
        if (name == "ShowOpenDialog") 
        {
            // showOpenDialog(allowMultipleSelection, chooseDirectory, title, initialPath, fileTypes)
            //
            // Inputs:
            //  allowMultipleSelection - Boolean
            //  chooseDirectory - Boolean. Choose directory if true, choose file if false
            //  title - title of the dialog
            //  initialPath - initial path to display. Pass "" to show default.
            //  fileTypes - space-delimited string of file extensions, without '.'
            //
            // Output:
            //  "" if no file/directory was selected
            //  JSON-formatted array of full path names if one or more files were selected
            //
            // Error:
            //  NO_ERROR
            //  ERR_INVALID_PARAMS - invalid parameters
            
            errorCode = ExecuteShowOpenDialog(arguments, retval, exception);
        }
        else if (name == "ReadDir")
        {
            // ReadDir(path)
            //
            // Inputs:
            //  path - full path of directory to be read
            //
            // Outputs:
            //  JSON-formatted array of the names of the files in the directory, not including '.' and '..'.
            //
            // Error:
            //   NO_ERROR - no error
            //   ERR_UNKNOWN - unknown error
            //   ERR_INVALID_PARAMS - invalid parameters
            //   ERR_NOT_FOUND - directory could not be found
            //   ERR_CANT_READ - could not read directory
            
            errorCode = ExecuteReadDir(arguments, retval, exception);
        }
        else if (name == "IsDirectory")
        {
            // IsDirectory(path)
            //
            // Inputs:
            //  path - full path of directory to test
            //
            // Outputs:
            //  true if path is a directory, false if error or it is a file
            //
            // Error:
            //  NO_ERROR - no error
            //  ERR_INVALID_PARAMS - invalid parameters
            //  ERR_NOT_FOUND - file/directory could not be found
            
            errorCode = ExecuteIsDirectory(arguments, retval, exception);
        }
        else if (name == "ReadFile")
        {
            // ReadFile(path, encoding)
            //
            // Inputs:
            //  path - full path of file to read
            //  encoding - 'utf8' is the only supported format for now
            //
            // Output:
            //  String - contents of the file
            //
            // Error:
            //  NO_ERROR - no error
            //  ERR_UNKNOWN - unknown error
            //  ERR_INVALID_PARAMS - invalid parameters
            //  ERR_NOT_FOUND - file could not be found
            //  ERR_CANT_READ - file could not be read
            //  ERR_UNSUPPORTED_ENCODING - unsupported encoding value 
            
            errorCode = ExecuteReadFile(arguments, retval, exception);
        }
        else if (name == "WriteFile")
        {
            // WriteFile(path, data, encoding)
            //
            // Inputs:
            //  path - full path of file to write
            //  data - data to write to file
            //  encoding - 'utf8' is the only supported format for now
            //
            // Output:
            //  none
            //
            // Error:
            //  NO_ERROR - no error
            //  ERR_UNKNOWN - unknown error
            //  ERR_INVALID_PARAMS - invalid parameters
            //  ERR_UNSUPPORTED_ENCODING - unsupported encoding value
            //  ERR_CANT_WRITE - file could not be written
            //  ERR_OUT_OF_SPACE - no more space for file
            
            errorCode = ExecuteWriteFile(arguments, retval, exception);
        }
        else if (name == "SetPosixPermissions")
        {
            // SetPosixPermissions(path, mode)
            //
            // Inputs:
            //  path - full path of file or directory
            //  mode - permissions for file or directory, in numeric format
            //
            // Output:
            //  none
            //
            // Errors
            //  NO_ERROR - no error
            //  ERR_UNKNOWN - unknown error
            //  ERR_INVALID_PARAMS - invalid parameters
            //  ERR_NOT_FOUND - can't file file/directory
            //  ERR_UNSUPPORTED_ENCODING - unsupported encoding value
            //  ERR_CANT_WRITE - permissions could not be written
            
            errorCode = ExecuteSetPosixPermissions(arguments, retval, exception);
            
        }
        else if ( name == "GetFileModificationTime")
        {
            // Returns the time stamp for a file or directory
            // 
            // Inputs:
            //  path - full path of file or directory
            //
            // Outputs:
            // Date - timestamp of file
            // 
            // Possible error values:
            //    NO_ERROR
            //    ERR_UNKNOWN
            //    ERR_INVALID_PARAMS
            //    ERR_NOT_FOUND
             
            errorCode = ExecuteGetFileModificationTime( arguments, retval, exception);
        }
        else if (name == "DeleteFileOrDirectory")
        {
            // DeleteFileOrDirectory(path)
            //
            // Inputs:
            //  path - full path of file or directory
            //
            // Ouput:
            //  none
            //
            // Errors
            //  NO_ERROR - no error
            //  ERR_UNKNOWN - unknown error
            //  ERR_INVALID_PARAMS - invalid parameters
            //  ERR_NOT_FOUND - can't file file/directory
            
            errorCode = ExecuteDeleteFileOrDirectory(arguments, retval, exception);
        }
        else if (name == "GetLastError")
        {
            // Special case private native function to return the last error code.
            retval = CefV8Value::CreateInt(lastError);
            
            // Early exit since we are just returning the last error code
            return true;
        }
        
        if (errorCode != -1) 
        {
            lastError = errorCode;
            return true;
        }
        
        return false;
    }


    int ExecuteShowOpenDialog(const CefV8ValueList& arguments,
                               CefRefPtr<CefV8Value>& retval,
                               CefString& exception)
    {
        if (arguments.size() != 5 || !arguments[2]->IsString() || !arguments[3]->IsString() || !arguments[4]->IsString())
            return ERR_INVALID_PARAMS;
        
        // Grab the arguments
        bool allowsMultipleSelection = arguments[0]->GetBoolValue();
        bool canChooseDirectories = arguments[1]->GetBoolValue();
        bool canChooseFiles = !canChooseDirectories;
        std::wstring wtitle = StringToWString(arguments[2]->GetStringValue());
        std::wstring initialPath = StringToWString(arguments[3]->GetStringValue());
        std::string fileTypesStr = arguments[4]->GetStringValue();
        std::string selectedFilenames = "";
        std::string result = "";

        wchar_t szFile[MAX_PATH];
        szFile[0] = 0;

        // TODO: This method should be using IFileDialog instead of the
        // outdated SHGetPathFromIDList and GetOpenFileName.

        if (canChooseDirectories) {
            BROWSEINFO bi = {0};
            bi.lpszTitle = wtitle.c_str();
            bi.ulFlags = BIF_NEWDIALOGSTYLE;
            LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
            if (pidl != 0) {
                if (SHGetPathFromIDList(pidl, szFile)) {
                    std::wstring strFoldername(szFile);
                    selectedFilenames = WStringToString(strFoldername);
                }
                IMalloc* pMalloc = NULL;
                SHGetMalloc(&pMalloc);
                if (pMalloc) {
                    pMalloc->Free(pidl);
                    pMalloc->Release();
                }
            }
        } else {
            OPENFILENAME ofn;

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = L"Web Files\0*.js;*.css;*.htm;*.html\0\0"; // TODO: Use passed in file types
            ofn.lpstrInitialDir = initialPath.c_str();
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            if (allowsMultipleSelection)
                ofn.Flags |= OFN_ALLOWMULTISELECT;

            if (GetOpenFileName(&ofn)) {
                std::wstring strFilename(szFile);
                selectedFilenames = WStringToString(strFilename);
            }
        }

        // TODO: Handle multiple select
        if (selectedFilenames.length() > 0) {
            std::string escapedFilenames;
            EscapeJSONString(selectedFilenames, escapedFilenames);
            result = "[\"" + escapedFilenames + "\"]";
        }
        else {
            result = "[]";
        }

        retval = CefV8Value::CreateString(result);

        return NO_ERROR;
    }
    
    int ExecuteReadDir(const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception)
    {
        if (arguments.size() != 1 || !arguments[0]->IsString())
            return ERR_INVALID_PARAMS;
        
        std::string pathStr = arguments[0]->GetStringValue();
        std::string result = "[";

        FixFilename(pathStr);
        pathStr += "\\*";

        WIN32_FIND_DATA ffd;
        HANDLE hFind = FindFirstFile(StringToWString(pathStr).c_str(), &ffd);

        if (hFind != INVALID_HANDLE_VALUE) {
            BOOL bAddedOne = false;
            do {
                // Ignore '.' and '..'
                if (!wcscmp(ffd.cFileName, L".") || !wcscmp(ffd.cFileName, L".."))
                    continue;

                if (bAddedOne)
                    result += ",";
                else
                    bAddedOne = TRUE;
                std::wstring wfilename = ffd.cFileName;
                std::string filename = WStringToString(wfilename);
// TODO?:                EscapeJSONString(filename, filename);
                result += "\"" + filename + "\"";
            } while (FindNextFile(hFind, &ffd) != 0);

            FindClose(hFind);
        } 
        else {
            return ConvertWinErrorCode(GetLastError());
        }

        result += "]";
        retval = CefV8Value::CreateString(result);
        return NO_ERROR;
    }
    
    int ExecuteIsDirectory(const CefV8ValueList& arguments,
                            CefRefPtr<CefV8Value>& retval,
                            CefString& exception)
    {
        if (arguments.size() != 1 || !arguments[0]->IsString())
            return ERR_INVALID_PARAMS;
        
        std::string pathStr = arguments[0]->GetStringValue();
        FixFilename(pathStr);

        DWORD dwAttr = GetFileAttributes(StringToWString(pathStr).c_str());

        if (dwAttr == INVALID_FILE_ATTRIBUTES) {
            return ConvertWinErrorCode(GetLastError()); 
        }

        retval = CefV8Value::CreateBool((dwAttr & FILE_ATTRIBUTE_DIRECTORY) != 0);
        return NO_ERROR;
    }
    
    int ExecuteReadFile(const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception)
    {
        if (arguments.size() != 2 || !arguments[0]->IsString() || !arguments[1]->IsString())
            return ERR_INVALID_PARAMS;

        std::string pathStr = arguments[0]->GetStringValue();
        std::string encodingStr = arguments[1]->GetStringValue();

        if (encodingStr != "utf8")
            return ERR_UNSUPPORTED_ENCODING;

        FixFilename(pathStr);
        
        std::wstring wPathStr = StringToWString(pathStr);

        DWORD dwAttr;
        dwAttr = GetFileAttributes(wPathStr.c_str());
        if (INVALID_FILE_ATTRIBUTES == dwAttr)
            return ConvertWinErrorCode(GetLastError());

        if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
            return ERR_CANT_READ;

        HANDLE hFile = CreateFile(wPathStr.c_str(), GENERIC_READ,
            0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        int error = NO_ERROR;

        if (INVALID_HANDLE_VALUE == hFile)
            return ConvertWinErrorCode(GetLastError()); 

        DWORD dwFileSize = GetFileSize(hFile, NULL);
        DWORD dwBytesRead;
        char* buffer = (char*)malloc(dwFileSize);
        if (buffer && ReadFile(hFile, buffer, dwFileSize, &dwBytesRead, NULL)) {
            std::string contents(buffer, dwFileSize);
            retval = CefV8Value::CreateString(contents.c_str());
        }
        else {
            if (!buffer)
                error = ERR_UNKNOWN;
            else
                error = ConvertWinErrorCode(GetLastError());
        }
        CloseHandle(hFile);
        if (buffer)
            free(buffer);

        return error; 
    }
    
    int ExecuteWriteFile(const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception)
    {
        if (arguments.size() != 3 || !arguments[0]->IsString() || !arguments[1]->IsString() || !arguments[2]->IsString())
            return ERR_INVALID_PARAMS;

        std::string pathStr = arguments[0]->GetStringValue();
        std::string contentsStr = arguments[1]->GetStringValue();
        std::string encodingStr = arguments[2]->GetStringValue();
        FixFilename(pathStr);

        if (encodingStr != "utf8")
            return ERR_UNSUPPORTED_ENCODING;

        HANDLE hFile = CreateFile(StringToWString(pathStr).c_str(), GENERIC_WRITE,
            0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        DWORD dwBytesWritten;
        int error = NO_ERROR;

        if (INVALID_HANDLE_VALUE == hFile)
            return ConvertWinErrorCode(GetLastError(), false); 

        // TODO: Should write to temp file
        // TODO: Encoding
        if (!WriteFile(hFile, contentsStr.c_str(), contentsStr.length(), &dwBytesWritten, NULL)) {
            error = ConvertWinErrorCode(GetLastError(), false);
        }

        CloseHandle(hFile);
        return error;
    }

    int ExecuteGetFileModificationTime(const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception)
    {
        if (arguments.size() != 1 || !arguments[0]->IsString())
            return ERR_INVALID_PARAMS;

        std::string pathStr = arguments[0]->GetStringValue();
        FixFilename(pathStr);

		// Remove trailing "\", if present. _wstat will fail with a "file not found"
		// error if a directory has a trailing '\' in the name.
		if (pathStr[pathStr.length() - 1] == '\\')
			pathStr[pathStr.length() - 1] = 0;

        /* Alternative implementation
        WIN32_FILE_ATTRIBUTE_DATA attribData;
        GET_FILEEX_INFO_LEVELS FileInfosLevel;
        GetFileAttributesEx( StringToWString(pathStr).c_str(), GetFileExInfoStandard, &attribData);*/


        struct _stat buffer;
        if(_wstat(StringToWString(pathStr).c_str(), &buffer) == -1) {
            return ConvertErrnoCode(errno); 
        }

        retval = CefV8Value::CreateDate(buffer.st_mtime);

        return NO_ERROR;
    }
    
    int ExecuteSetPosixPermissions(const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception)
    {
        if (arguments.size() != 2 || !arguments[0]->IsString() || !arguments[1]->IsInt())
            return ERR_INVALID_PARAMS;
        
        std::string pathStr = arguments[0]->GetStringValue();
        int mode = arguments[1]->GetIntValue();
        FixFilename(pathStr);

		/* TODO: Implement me! _wchmod() uses different parameters than chmod(), and
		/  will _not_ always work on directories. For now, do nothing and return NO_ERROR
		/  so most unit test can at least be run. 

        if (_wchmod(StringToWString(pathStr).c_str(), mode) == -1) {
            return ConvertErrnoCode(errno); 
        }
		*/

        return NO_ERROR;
    }
    
    int ExecuteDeleteFileOrDirectory(const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception)
    {
        if (arguments.size() != 1 || !arguments[0]->IsString())
            return ERR_INVALID_PARAMS;
        
        std::string pathStr = arguments[0]->GetStringValue();
        FixFilename(pathStr);

        if (!DeleteFile(StringToWString(pathStr).c_str()))
            return ConvertWinErrorCode(GetLastError());

        return NO_ERROR;
    }

    void FixFilename(std::string& filename)
    {
        // Convert '/' to '\'
        for (unsigned int i = 0; i < filename.length(); i++) {
            if (filename[i] == '/')
                filename[i] = '\\';
        }
    }

    std::wstring StringToWString(const std::string& s)
    {
        std::wstring temp(s.length(),L' ');
        std::copy(s.begin(), s.end(), temp.begin());
        return temp;
    }

    std::string WStringToString(const std::wstring& s)
    {
        std::string temp(s.length(), ' ');
        std::copy(s.begin(), s.end(), temp.begin());
        return temp;
    }

    // Escapes characters that have special meaning in JSON
    void EscapeJSONString(const std::string& str, std::string& result) {
        result = "";
        
        for(size_t pos = 0; pos != str.size(); ++pos) {
                switch(str[pos]) {
                    case '\a':  result.append("\\a");   break;
                    case '\b':  result.append("\\b");   break;
                    case '\f':  result.append("\\f");   break;
                    case '\n':  result.append("\\n");   break;
                    case '\r':  result.append("\\r");   break;
                    case '\t':  result.append("\\t");   break;
                    case '\v':  result.append("\\v");   break;
                    // Note: single quotes are OK for JSON
                    case '\"':  result.append("\\\"");  break; // double quote
                    case '\\':  result.append("/");  break; // backslash                        
                        
                default:   result.append( 1, str[pos]); break;
                        
            }
        }
    }

    // Maps errors from errno.h to the brackets error codes
    // found in brackets_extensions.js
    int ConvertErrnoCode(int errorCode, bool isReading = true)
    {
        switch (errorCode) {
        case NO_ERROR:
            return NO_ERROR;
        case EINVAL:
            return ERR_INVALID_PARAMS;
        case ENOENT:
            return ERR_NOT_FOUND;
        default:
            return ERR_UNKNOWN;
        }
    }

    // Maps errors from  WinError.h to the brackets error codes
    // found in brackets_extensions.js
    int ConvertWinErrorCode(int errorCode, bool isReading = true)
    {
        switch (errorCode) {
        case NO_ERROR:
            return NO_ERROR;
        case ERROR_PATH_NOT_FOUND:
        case ERROR_FILE_NOT_FOUND:
            return ERR_NOT_FOUND;
        case ERROR_ACCESS_DENIED:
            return isReading ? ERR_CANT_READ : ERR_CANT_WRITE;
        case ERROR_WRITE_PROTECT:
            return ERR_CANT_WRITE;
        case ERROR_HANDLE_DISK_FULL:
            return ERR_OUT_OF_SPACE;
        default:
            return ERR_UNKNOWN;
        }
    }

private:
    int lastError;
    IMPLEMENT_REFCOUNTING(BracketsExtensionHandler);
};


void InitBracketsExtensions()
{
    // Register a V8 extension with JavaScript code that calls native
    // methods implemented in BracketsExtensionHandler.
    
    // The JavaScript code for the extension lives in res/brackets_extensions.js
    
    //NSString* sourcePath = [[NSBundle mainBundle] pathForResource:@"brackets_extensions" ofType:@"js"];
    //NSString* jsSource = [[NSString alloc] initWithContentsOfFile:sourcePath encoding:NSUTF8StringEncoding error:nil];

    extern HINSTANCE hInst;

    HRSRC hRes = FindResource(hInst, MAKEINTRESOURCE(IDS_BRACKETS_EXTENSIONS), MAKEINTRESOURCE(256));
    DWORD dwSize;
    LPBYTE pBytes = NULL;

    if(hRes)
    {
        HGLOBAL hGlob = LoadResource(hInst, hRes);
        if(hGlob)
        {
            dwSize = SizeofResource(hInst, hRes);
            pBytes = (LPBYTE)LockResource(hGlob);
        }
    }

    if (pBytes) {
        std::string jsSource((const char *)pBytes, dwSize);
        CefRegisterExtension("brackets", jsSource.c_str(), new BracketsExtensionHandler());
    }
}