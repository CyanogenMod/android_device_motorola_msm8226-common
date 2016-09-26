#define LOG_TAG "RilWrapper"
#define RIL_SHLIB

#include <dlfcn.h>
#include <log/log.h>
#include <telephony/ril.h>

#define REAL_RIL_NAME "libril-qc-qmi-1.so"

static RIL_RadioFunctions const *mRealRadioFuncs;
static const struct RIL_Env *mEnv;

const RIL_RadioFunctions* RIL_Init(const struct RIL_Env *env, int argc, char **argv)
{
	RIL_RadioFunctions const* (*fRealRilInit)(const struct RIL_Env *env, int argc, char **argv);
	static RIL_RadioFunctions rilInfo;
	void *realRilLibHandle;
	int i;

	// save the env;
	mEnv = env;

	// get the real RIL
	realRilLibHandle = dlopen(REAL_RIL_NAME, RTLD_LOCAL);
	if (!realRilLibHandle) {
		RLOGE("Failed to load the real RIL '" REAL_RIL_NAME  "': %s\n", dlerror());
		return NULL;
	}

	// load the real RIL
	fRealRilInit = dlsym(realRilLibHandle, "RIL_Init");
	if (!fRealRilInit) {
		RLOGE("Failed to find the real RIL's entry point\n");
		goto out_fail;
	}

	RLOGD("Calling the real RIL's entry point with %u args\n", argc);
	for (i = 0; i < argc; i++)
		RLOGD("  argv[%2d] = '%s'\n", i, argv[i]);

	// try to init the real ril
	mRealRadioFuncs = fRealRilInit(env, argc, argv);
	if (!mRealRadioFuncs) {
		RLOGE("The real RIL's entry point failed\n");
		goto out_fail;
	}

	// copy the real RIL's info struct, then replace the onRequest pointer with our own
	rilInfo = *mRealRadioFuncs;
	rilInfo.version = 12;

	RLOGD("Wrapped RIL version is '%s'\n", mRealRadioFuncs->getVersion());

	// we're all good - return to caller
	return &rilInfo;

out_fail:
	dlclose(realRilLibHandle);
	return NULL;
}
