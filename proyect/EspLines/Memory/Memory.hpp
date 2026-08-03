#pragma once
#include <cstdint>
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>

namespace Memory {

	class MemoryReader {
	public:
		template<typename T>
		T Read(std::uintptr_t address) const {
			T value{};
			if (m_ProcessHandle) {
				SIZE_T read = 0;
				ReadProcessMemory(m_ProcessHandle, reinterpret_cast<LPCVOID>(address), &value, sizeof(T), &read);
			}
			return value;
		}

		template<typename T>
		bool Write(std::uintptr_t address, const T& value) const {
			if (!m_ProcessHandle)
				return false;
			SIZE_T written = 0;
			return WriteProcessMemory(m_ProcessHandle, reinterpret_cast<LPVOID>(address), &value, sizeof(T), &written) != FALSE;
		}

		bool Attach(const std::wstring& processName) {
			Detach();
			HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (snapshot == INVALID_HANDLE_VALUE)
				return false;

			PROCESSENTRY32W entry;
			entry.dwSize = sizeof(PROCESSENTRY32W);
			if (Process32FirstW(snapshot, &entry)) {
				do {
					if (processName == entry.szExeFile) {
						m_ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
						CloseHandle(snapshot);
						return m_ProcessHandle != nullptr;
					}
				} while (Process32NextW(snapshot, &entry));
			}
			CloseHandle(snapshot);
			return false;
		}

		void Detach() {
			if (m_ProcessHandle) {
				CloseHandle(m_ProcessHandle);
				m_ProcessHandle = nullptr;
			}
		}

		~MemoryReader() {
			Detach();
		}

	private:
		HANDLE m_ProcessHandle = nullptr;
	};

}

inline Memory::MemoryReader Mem;
