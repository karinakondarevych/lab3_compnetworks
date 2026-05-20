#include <windows.h>
#include <iostream>
#include <string>
#include <thread>

using namespace std;

#define PIPE_NAME L"\\\\.\\pipe\\ForumPipe"
#define MAILSLOT_NAME L"\\\\.\\mailslot\\forum_discovery"

void Receiver(HANDLE hPipe) { // потік для постійного прийому повідомлень від сервера (інших користувачів)
    char buf[1024];
    DWORD read;
    while (true) {
        DWORD avail = 0;
        if (PeekNamedPipe(hPipe, NULL, 0, NULL, &avail, NULL) && avail > 0) { // перевірка, чи сервер надіслав щось
            if (ReadFile(hPipe, buf, sizeof(buf) - 1, &read, NULL) && read > 0) {
                buf[read] = '\0';
                cout << "\n" << buf << "\n> ";
            }
        }
        if (GetLastError() == ERROR_BROKEN_PIPE) // якщо сервер закрився або канал обірвався
            break;
        Sleep(10);
    }
}

int main() {
    cout << "Enter your username: ";
    string name;
    getline(cin, name);

    // Discovery (виявлення сервера) через Mailslot
    HANDLE hSlot = CreateFile(MAILSLOT_NAME, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL); //  відкриття Mailslot як звичайного файлу для запису
    if (hSlot != INVALID_HANDLE_VALUE) {
        string ping = "Hello from " + name;
        DWORD w;
        WriteFile(hSlot, ping.c_str(), (DWORD)ping.length(), &w, NULL);
        CloseHandle(hSlot);
        cout << "[System] Server discovery signal sent." << endl;
    }

    // підключення до іменованого каналу (Named Pipe)
    WaitNamedPipe(PIPE_NAME, 3000);
    HANDLE hPipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
        cout << "Connection failed! Make sure the server is running." << endl;
        return 1;
    }

    cout << "Connected! Type messages below (type 'exit' to quit):" << endl;

    thread(Receiver, hPipe).detach();     // окремий потік для отримання повідомлень

    string input;
    while (true) {
        getline(cin, input);

        if (input == "exit")
            break;

        string full = name + ": " + input;
        DWORD w;
        
        WriteFile(hPipe, full.c_str(), (DWORD)full.length(), &w, NULL); // повідомлення відправляється на сервер
    }

    CloseHandle(hPipe);
    return 0;
}