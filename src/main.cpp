#include "bugfix.h"
#include "gctracehooks.h"
#include "variabletracehooks.h"

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* F4SE, F4SE::PluginInfo* Info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format("{}.log", BUILD_PROJECT_NAME);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::warn);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("[%H:%M:%S:%e TID %5t] %v"s);

	spdlog::info("{} v{}.{}", BUILD_PROJECT_NAME, BUILD_VERSION_MAJOR, BUILD_VERSION_MINOR);

	Info->infoVersion = F4SE::PluginInfo::kVersion;
	Info->name = BUILD_PROJECT_NAME;
	Info->version = BUILD_VERSION_MAJOR;

	if (F4SE->IsEditor()) {
		logger::critical("Loaded in editor"sv);
		return false;
	}

	const auto ver = F4SE->RuntimeVersion();

	if (ver < REL::Relocate(F4SE::RUNTIME_1_10_162, F4SE::RUNTIME_1_10_162, F4SE::RUNTIME_LATEST_VR)) {
		F4SE::stl::report_and_fail(
			fmt::format("{} does not support runtime v{}.",
				Info->name,
				ver.string()));
	}

	return true;
}

extern "C" DLLEXPORT constinit auto F4SEPlugin_Version = []() noexcept {
	F4SE::PluginVersionData data = {};

	data.PluginVersion({ BUILD_VERSION_MAJOR, BUILD_VERSION_MINOR, 0 });
	data.PluginName(BUILD_PROJECT_NAME);
	data.AuthorName("Nukem");
	data.UsesAddressLibrary(true);
	data.UsesSigScanning(false);
	data.IsLayoutDependent(true);
	data.HasNoStructUse(false);
	data.CompatibleVersions({ F4SE::RUNTIME_1_10_162, F4SE::RUNTIME_1_10_163 });

	return data;
}();

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* F4SE)
{
	F4SE::Init(F4SE);

	auto iniPath = "Data/F4SE/Plugins/GCBugFix.ini";
	int logFlushTime = GetPrivateProfileIntA("GC", "LogFlushTime", 5, iniPath);
	int logGCPasses = GetPrivateProfileIntA("GC", "LogPasses", 0, iniPath);
	int traceGCVariables = GetPrivateProfileIntA("GC", "TraceVariables", 0, iniPath);
	int applyFix = GetPrivateProfileIntA("GC", "ApplyFix", 1, iniPath);

	if (logGCPasses != 0 || traceGCVariables != 0)
		spdlog::flush_every(std::chrono::seconds(logFlushTime));

	logger::info("LogFlushTime = {}", logFlushTime);
	logger::info("LogPasses = {}", logGCPasses);
	logger::info("TraceVariables = {}", traceGCVariables);
	logger::info("ApplyFix = {}", applyFix);

	// Set up hooks
	auto& trampoline = F4SE::GetTrampoline();

	if (trampoline.empty())
		F4SE::AllocTrampoline(512);

	if (logGCPasses)
		GCTraceHooks::InstallHooks();

	if (traceGCVariables)
		VariableTraceHooks::InstallHooks();

	if (applyFix)
		Bugfix::InstallHooks();

	return true;
}
