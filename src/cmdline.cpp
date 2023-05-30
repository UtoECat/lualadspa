/*
 * LuaLadspa - write your own LADSPA plugins on lua :)
 * CLI(no :D) interface for managing existing plugins/debug and 
 * develop your ones.
 * (Actually just enumerates and checks plugins for consistency)
 * Copyright (C) 2023 UtoECat

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ladspa.h"
#include <dlfcn.h> // load it dynamicly

//extern "C" const LADSPA_Descriptor* ladspa_descriptor(unsigned long Index);

#include <cstdio>
#include <string>
#include <cstdarg>
#include <memory>

FILE* outlog = stderr;

// copy pasted here to be independent form loaded shared object
const std::string vstrformat(const char * const fmt, va_list args) {
	va_list args2;
	va_copy(args2, args);
	const int len = std::vsnprintf(NULL, 0, fmt, args2);
	va_end(args2);
	
	if (len <= 0) throw "formatting error";
	auto tmp = std::make_unique<char[]>(len+1);

	std::vsnprintf(tmp.get(), len+1, fmt, args);
	return std::string(tmp.get(), len);
}

void logError(const char* message, ...) {
	va_list args;
	va_start(args, message);
	const std::string res = vstrformat(message, args);
	fprintf(outlog, "[Error]: ");
	fwrite(res.c_str(), 1, res.size(), outlog);
	fprintf(outlog, "\n");
	va_end(args);
	return;
}

void logInfo(const char* message, ...) {
	va_list args;
	va_start(args, message);
	const std::string res = vstrformat(message, args);
	fprintf(outlog, "[Info ]: ");
	fwrite(res.c_str(), 1, res.size(), outlog);
	fprintf(outlog, "\n");
	va_end(args);
	return;
}

typedef FILE* (*logSetter)(FILE* f);

int main() {

	void* handle = dlopen("liblualadspa.so", RTLD_NOW);
	if (!handle) {
		logError("Can't open lualadspa.so : %s!", dlerror());
		return -1;
	}

	void* ptr = dlsym(handle, "ladspa_descriptor");
	LADSPA_Descriptor_Function ladspa_descriptor = 
		reinterpret_cast<LADSPA_Descriptor_Function>(ptr);

	ptr = dlsym(handle, "setlogdesc");
	logSetter setLog = reinterpret_cast<logSetter>(ptr);

	if (!ladspa_descriptor || !setLog) {
		logError("Invalid lualadspa.so : %s!", dlerror());
		return -1;
	}

	FILE* old = setLog(stderr);
	if (old != stderr) fclose(old);

	int ind = 0;
	const LADSPA_Descriptor* D;
	LADSPA_Data tmp[128];

	while ((D = ladspa_descriptor(ind)) != nullptr) {
		logInfo("Checking plugin %s...", D->Label);
		auto inst = D->instantiate(D, 65535);
		logInfo("name %s, label %s, portsCount %i, index %i", D->Name, D->Label,
				D->PortCount, ind);

		if (inst != nullptr) {
			D->activate(inst);
			for (int i = 0; i < D->PortCount; i++) {
				D->connect_port(inst, i, tmp);
				logInfo("Port %i : name %s, mode %c%c", i, D->PortNames[i],
					D->PortDescriptors[i] & LADSPA_PORT_INPUT ?
					'i' : (D->PortDescriptors[i] & LADSPA_PORT_OUTPUT ? 'o' : '?'),
					D->PortDescriptors[i] & LADSPA_PORT_CONTROL ?
					'c' : (D->PortDescriptors[i] & LADSPA_PORT_AUDIO ? 'a' : '?'));
				int h = D->PortRangeHints[i].HintDescriptor;
				if (h != 0) {
					logInfo("Port %i hints : min[%c], max[%c], bool[%c]"
							", int[%c], log[%c], freq[%c]", i, 
							(h & LADSPA_HINT_BOUNDED_BELOW)?'+':'-',
							(h & LADSPA_HINT_BOUNDED_ABOVE)?'+':'-',
							(h & LADSPA_HINT_TOGGLED)?'+':'-',
							(h & LADSPA_HINT_INTEGER)?'+':'-',
							(h & LADSPA_HINT_LOGARITHMIC)?'+':'-',
							(h & LADSPA_HINT_SAMPLE_RATE)?'+':'-'
						);
				}
			}
			D->run(inst, 128);
			D->run(inst, 128);
			D->run(inst, 128);
			logInfo("Reactivation, and run 4 times again...");
			D->deactivate(inst);
			D->activate(inst);
			D->run(inst, 128);
			D->run(inst, 128);
			D->run(inst, 128);
			D->run(inst, 128);
			D->deactivate(inst);
			logInfo("Deactivation...");
			D->cleanup(inst);	
		} else logError("Can't instantiate plugin %s!", D->Name);
		ind++;
		logInfo("----------------------------------------------");
	}
	if (ladspa_descriptor(0) == nullptr) 
		logError("No lualadspa plugin founded!");	
	dlclose(handle);
	return 0;
}
