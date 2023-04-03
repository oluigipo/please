#include "build.h"

#ifdef ENV__DEFAULT
#   define ENV_ANDROID_SDK "B:/programs/android/sdk"
#   define ENV_ANDROID_NDK "B:/programs/android/ndk-r23c"
#   define ENV_JAVA_JDK "B:/programs/jdk-17.0.6+10"
#endif

#ifndef ENV_ANDROID_SDK
#   error "please define ENV_ANDROID_SDK"
#endif
#ifndef ENV_ANDROID_NDK
#   error "please define ENV_ANDROID_NDK"
#endif
#ifndef ENV_JAVA_JDK
#   error "please define ENV_JAVA_JDK"
#endif

struct
{
	struct Build_Executable* exec;
	int optimize;
	bool debug_mode;
	bool embed;
	
	bool install;
	bool launch;
}
static g_opts = {
	.exec = &g_executables[0],
	.debug_mode = true,
};

static bool
CreateMakefiles(void)
{
	if (!CreateDirIfNeeded(LocalPrintf(64, "%s/jni", g_builddir)))
		return false;
	
	FILE* file;
	
	//- NOTE(ljre): Android.mk
	file = fopen(LocalPrintf(256, "%s/jni/Android.mk", g_builddir), "wb");
	if (!file)
		return false;
	
	fprintf(file,
		"# https://developer.android.com/ndk/guides/android_mk.html\n"
		"\n"
		"LOCAL_PATH := $(call my-dir)\n"
		"include $(CLEAR_VARS)\n"
		"\n"
		"LOCAL_MODULE := %s\n"
		"LOCAL_C_INCLUDES := ../include ../src\n"
		"LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv3 -lSDL2\n"
		"LOCAL_STATIC_LIBRARIES := android_native_app_glue\n"
		"\n", g_opts.exec->outname);
	
	fprintf(file, "LOCAL_SRC_FILES :=");
	for (struct Build_Tu** tu = g_opts.exec->tus; *tu; ++tu)
	{
		fprintf(file, " ../../src/%s", (*tu)->path);
	}
	fprintf(file, "\n");
	
	fprintf(file, "LOCAL_CFLAGS := -DCONFIG_OS_ANDROID");
	if (g_opts.embed)
		fprintf(file, " -DCONFIG_DEBUG");
	for (Cstr* def = g_opts.exec->defines; def && *def; ++def)
		fprintf(file, " -D%s", *def);
	fprintf(file, "\n");
	
	fprintf(file,
		"\n"
		"include $(BUILD_SHARED_LIBRARY)\n"
		"$(call import-module,android/native_app_glue)\n");
	
	fclose(file);
	
	//- NOTE(ljre): Application.mk
	file = fopen(LocalPrintf(256, "%s/jni/Application.mk", g_builddir), "wb");
	if (!file)
		return false;
	
	fprintf(file,
		"# https://developer.android.com/ndk/guides/application_mk.html\n"
		"\n"
		"NDK_TOOLCHAIN_VERSION := clang\n"
		"APP_PLATFORM := android-18\n"
		"APP_OPTIM := %s\n"
		"APP_ABI := armeabi-v7a # arm64-v8a x86\n"
		"\n", g_opts.optimize > 0 ? "release" : "debug");
	
	fclose(file);
	
	return true;
}

static bool
CallNdkBuildSystem(void)
{
	return RunCommand(
		LocalPrintf(
		512,
		"cd %s && \"" ENV_ANDROID_NDK "/ndk-build.cmd\" -j1 NDK_LIBS_OUT=lib\\lib NDK_PROJECT_PATH=.",
		g_builddir));
}

static bool
CallAapt(void)
{
	return false;
}

static bool
CreateKeystoreIfNeeded(void)
{
	return false;
}

static bool
SignAndZipApk(void)
{
	return false;
}

static int PrintHelp(void);

int
main(int argc, char** argv)
{
	g_self = argv[0];
	g_target = "armeabi_v7a-android";
	
	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "/?") || !strcmp(argv[i], "-help") || !strcmp(argv[i], "--help"))
			return PrintHelp();
		else if (strcmp(argv[i], "-g") == 0)
		{}
		else if (strcmp(argv[i], "-install") == 0)
			g_opts.install = true;
		else if (strcmp(argv[i], "-launch") == 0)
			g_opts.launch = true;
		else if (strcmp(argv[i], "-ndebug") == 0)
			g_opts.debug_mode = false;
		else if (strcmp(argv[i], "-embed") == 0)
			g_opts.embed = true;
		else if (strncmp(argv[i], "-O", 2) == 0)
		{
			char* end;
			int opt = strtol(argv[i]+2, &end, 10);
			
			if (end == argv[i]+strlen(argv[i]))
				g_opts.optimize = opt;
			else
				fprintf(stderr, "[warning] invalid optimization flag '%s'.\n", argv[i]);
		}
		else if (argv[i][0] == '-')
			fprintf(stderr, "[warning] unknown flag '%s'.\n", argv[i]);
		else
		{
			struct Build_Executable* exec = NULL;
			
			for (int j = 0; j < ArrayLength(g_executables); ++j)
			{
				if (strcmp(g_executables[j].name, argv[i]) == 0)
				{
					exec = &g_executables[j];
					break;
				}
			}
			
			if (exec)
				g_opts.exec = exec;
			else
				fprintf(stderr, "[warning] unknown project '%s'.\n", argv[i]);
		}
	}
	
	bool ok = true;
	
	ok = ok && CreateDirIfNeeded(g_builddir);
	ok = ok && CreateMakefiles();
	ok = ok && CallNdkBuildSystem();
	ok = ok && CallAapt();
	ok = ok && CreateKeystoreIfNeeded();
	ok = ok && SignAndZipApk();
	
	return !ok;
}

static int
PrintHelp(void)
{
	fprintf(stdout,
		"usage: %s PROJECT_NAME [OPTIONS...]\n"
		"default project: %s\n"
		"\n"
		"Target:        %s\n"
		"\n"
		"OPTIONS:\n"
		"    -g                  Nothing for now\n"
		"    -ndebug             Don't define CONFIG_DEBUG macro\n"
		"    -embed              Embed assets in executable file (doesn't work on MSVC)\n"
		"    -O0 -O1 -O2         Optimization flags\n"
		"    -profile=release    alias for: -ndebug -O2 -embed\n"
		"",
		g_self, g_opts.exec->name, g_target);
	
	return 0;
}
