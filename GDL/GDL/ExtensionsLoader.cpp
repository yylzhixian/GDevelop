#include "GDL/ExtensionsLoader.h"
#include "GDL/ExtensionsManager.h"
#include <stdio.h>
#include <sys/types.h>

//Compiler specific include, for listing files of directory ( see below )
#if defined(__GNUC__)
#include <dirent.h>
#elif defined(_MSC_VER)
#include <windows.h>
#endif

#include <boost/version.hpp>

#include "GDL/BaseObjectExtension.h"
#include "GDL/AudioExtension.h"
#include "GDL/MouseExtension.h"
#include "GDL/KeyboardExtension.h"
#include "GDL/JoystickExtension.h"
#include "GDL/TimeExtension.h"
#include "GDL/FileExtension.h"
#include "GDL/VariablesExtension.h"
#include "GDL/CameraExtension.h"
#include "GDL/WindowExtension.h"
#include "GDL/NetworkExtension.h"
#include "GDL/SpriteExtension.h"
#include "GDL/SceneExtension.h"
#include "GDL/AdvancedExtension.h"
#include "GDL/Object.h"
#include "GDL/Game.h"
#include "GDL/Version.h"
#include "GDL/ExtensionBase.h"

#if defined(GDE)
#include <wx/log.h>
#endif

#if defined(WINDOWS)
    #include <windows.h>
    typedef HINSTANCE Handle;
    Handle OpenLibrary(const char* path) {return LoadLibrary(path);}
    void* GetSymbol(Handle library, const char* name) { return (void*)GetProcAddress(library, name);}
    void CloseLibrary(Handle library) {FreeLibrary(library);}
#elif defined(LINUX)
    #include <dlfcn.h>
    typedef void* Handle;
    Handle OpenLibrary(const char* path) {return dlopen(path, RTLD_LAZY);}
    void* GetSymbol(Handle library, const char* name) { return dlsym(library, name);}
    void CloseLibrary(Handle library) {dlclose(library);}
#else
    #error No system defined
#endif

typedef ExtensionBase* (*createExtension)();
typedef void (*destroyExtension)(ExtensionBase*);

using namespace gdp;

ExtensionsLoader::ExtensionsLoader() :
directory("./")
{
    //ctor
}

ExtensionsLoader::~ExtensionsLoader()
{
    //dtor
}

ExtensionsLoader *ExtensionsLoader::_singleton = NULL;

void ExtensionsLoader::LoadAllExtensionsAvailable()
{
    gdp::ExtensionsManager * extensionsManager = gdp::ExtensionsManager::getInstance();

    //Built-in extensions
    extensionsManager->AddExtension(boost::shared_ptr<ExtensionBase>(new ExtensionSprite()) );
    extensionsManager->AddExtension(boost::shared_ptr<ExtensionBase>(new AudioExtension()));
    extensionsManager->AddExtension(boost::shared_ptr<ExtensionBase>(new MouseExtension()));
    extensionsManager->AddExtension(boost::shared_ptr<ExtensionBase>(new KeyboardExtension()));
    extensionsManager->AddExtension(boost::shared_ptr<ExtensionBase>(new JoystickExtension()));
    extensionsManager->AddExtension(boost::shared_ptr<ExtensionBase>(new TimeExtension()));
    extensionsManager->AddExtension(boost::shared_ptr<ExtensionBase>(new FileExtension()));
    extensionsManager->AddExtension(boost::shared_ptr<ExtensionBase>(new VariablesExtension()));
    extensionsManager->AddExtension(boost::shared_ptr<ExtensionBase>(new CameraExtension()));
    extensionsManager->AddExtension(boost::shared_ptr<ExtensionBase>(new WindowExtension()));
    extensionsManager->AddExtension(boost::shared_ptr<ExtensionBase>(new NetworkExtension()));
    extensionsManager->AddExtension(boost::shared_ptr<ExtensionBase>(new SceneExtension()));
    extensionsManager->AddExtension(boost::shared_ptr<ExtensionBase>(new AdvancedExtension()));

    string suffix = "";

    #if defined(WINDOWS)
        suffix += "w";
    #elif defined(LINUX)
        suffix += "l";
    #else
        #error No target system defined.
    #endif

    #if defined(GDE)
        suffix += "e";
    #endif

    //External extensions
	#if defined(__GNUC__) //For compilers with posix support
    struct dirent *lecture;
    DIR *rep;
    rep = opendir( directory.c_str() );
    int l = 0;

    if ( rep == NULL )
    {
        cout << "Unable to open Extensions directory." << endl;
        return;
    }

    while (( lecture = readdir( rep ) ) )
    {
        string lec = lecture->d_name;
        if ( lec != "." && lec != ".." && lec.find(".xgd"+suffix, lec.length()-4-suffix.length()) != string::npos)
        {
            //Charger l'extension
            LoadExtensionInManager(directory+"/"+lec);

            l++;
        }
    }

    closedir( rep );
	#elif defined(_MSC_VER)
	WIN32_FIND_DATA f;
	string dirPart = "/*.xgd";
	string dirComplete = directory + dirPart + suffix;
	HANDLE h = FindFirstFile(dirComplete.c_str(), &f);
	if(h != INVALID_HANDLE_VALUE)
	{
		do
		{
			LoadExtensionInManager(f.cFileName);
		} while(FindNextFile(h, &f));
	}
	#else
		#error Compiler not supported ( but might support one style of directory listing, update defines if necessary )
	#endif
}

////////////////////////////////////////////////////////////
/// Charge l'extension voulu
/// dans le gestionnaire d'extension pour le jeu
////////////////////////////////////////////////////////////
void ExtensionsLoader::LoadExtensionInManager(std::string fullpath)
{
    gdp::ExtensionsManager * extensionsManager = gdp::ExtensionsManager::getInstance();
    Handle extensionHdl = OpenLibrary(fullpath.c_str());
    if(extensionHdl == NULL){
       cout << "Unable to load extension " << fullpath << "." << endl;
    }
    else
    {
        createExtension create_extension = (createExtension)GetSymbol(extensionHdl, "CreateGDExtension");
        destroyExtension destroy_extension = (destroyExtension)GetSymbol(extensionHdl, "DestroyGDExtension");

        if ( create_extension == NULL || destroy_extension == NULL )
        {
            cout << "Unable to load extension " << fullpath << " ( no valid create/destroy functions )." << endl;
        }
        else
        {
            ExtensionBase * extensionPtr = create_extension();
            boost::shared_ptr<ExtensionBase> extension(extensionPtr, destroy_extension);

            string error;

            //Perform safety check about the compilation
            if ( !extensionPtr->compilationInfo.informationCompleted )
                error += "Compilation information not filled.\n";

            #if defined(GDE)
            if ( extensionPtr->compilationInfo.runtimeOnly )
                error += "Extension compiled for runtime only.\n";

            if ( extensionPtr->compilationInfo.wxWidgetsMajorVersion != wxMAJOR_VERSION ||
                      extensionPtr->compilationInfo.wxWidgetsMinorVersion != wxMINOR_VERSION ||
                      extensionPtr->compilationInfo.wxWidgetsReleaseNumber != wxRELEASE_NUMBER ||
                      extensionPtr->compilationInfo.wxWidgetsSubReleaseNumber != wxSUBRELEASE_NUMBER )
                error += "Not the same wxWidgets version.\n";
            #endif
            #if defined(__GNUC__)
            if ( extensionPtr->compilationInfo.gccMajorVersion != __GNUC__ ||
                      extensionPtr->compilationInfo.gccMinorVersion != __GNUC_MINOR__ ||
                      extensionPtr->compilationInfo.gccPatchLevel != __GNUC_PATCHLEVEL__ )
                error += "Not the same GNU Compiler version.\n";

            #endif
            if ( extensionPtr->compilationInfo.sfmlMajorVersion != 2 ||
                      extensionPtr->compilationInfo.sfmlMinorVersion != 0 )
                error += "Not the same SFML version.\n";

            if ( extensionPtr->compilationInfo.boostVersion != BOOST_VERSION )
                error += "Not the same Boost version.";

            if ( extensionPtr->compilationInfo.gdlVersion != RC_FILEVERSION_STRING)
                error += "Not the same GDL version.\n";

            if ( extensionPtr->compilationInfo.sizeOfpInt != sizeof(int*))
                error += "Not the same architecture.\n";

            if ( !error.empty() )
            {
                char beep = 7;
                cout << "-- WARNING ! --" << beep << endl;
                cout << "Bad extension " + fullpath + " loaded :\n" + error;
                cout << "---------------" << endl;

                #if defined(GDE) && defined(RELEASE)
                wxString userMsg = string(_("L'extension "))+ fullpath + string(_(" pr�sente des erreurs :\n")) + error + string(_("\nL'extension n'a pas �t� charg�e. Prenez contact avec le d�veloppeur pour plus d'informations." ));
                wxLogWarning(userMsg);
                #endif
                #if defined(RELEASE) //Load extension despite errors in non release build
                return;
                #endif
            }

            extensionsManager->AddExtension(extension);
            return;
        }
    }
}
