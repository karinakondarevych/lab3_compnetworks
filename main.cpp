#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>

using namespace std;

#define PIPE_NAME L"\\\\.\\pipe\\ForumPipe"
#define MAILSLOT_NAME L"\\\\.\\mailslot\\forum_discovery"

vector<HANDLE> clientPipes; // список усіх активних підключень для розсилки

void Broadcast(const string& msg) { // функція для розсилки повідомлення всім підключеним клієнтам
    for (HANDLE h : clientPipes) {
        DWORD written;
        WriteFile(h, msg.c_str(), (DWORD)msg.length(), &written, NULL);
    }
}

void HandleClient(HANDLE hPipe) { // функція для обробки кожного окремого клієнта
    char buffer[1024];
    DWORD bytesRead;
    
    DWORD dwMode = PIPE_READMODE_MESSAGE; // встановлення режиму повідомлень
    SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL);

    clientPipes.push_back(hPipe); // додавання клієнта до списку для розсилки
    cout << "[Server] Client connected." << endl;

    while (true) {
        DWORD bytesAvail = 0;
        if (PeekNamedPipe(hPipe, NULL, 0, NULL, &bytesAvail, NULL) && bytesAvail > 0) { // якщо дані є, читаємо їх
            if (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                buffer[bytesRead] = '\0'; // Додаємо символ кінця рядка
                cout << buffer << endl;   // Виводимо повідомлення в консоль сервера
                Broadcast(buffer);        // Розсилаємо всім учасникам форуму
            }
        }

        if (GetLastError() == ERROR_BROKEN_PIPE) // перевірка, чи не відключився клієнт
            break;
        Sleep(10);
    }

    cout << "[Server] Client disconnected." << endl;
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
}

void MailslotService() { // служба Mailslot, щоб клієнти могли знайти сервер у мережі
    HANDLE hSlot = CreateMailslot(MAILSLOT_NAME, 0, MAILSLOT_WAIT_FOREVER, NULL);
    char buf[256];
    DWORD read;
    while (ReadFile(hSlot, buf, sizeof(buf) - 1, &read, NULL)) {
        buf[read] = '\0';
        cout << "[Mailslot] " << buf << endl;
    }
}

int main() {
    cout << "--- FORUM SERVER ---" << endl;

    thread(MailslotService).detach();

    while (true) {
        HANDLE hPipe = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_DUPLEX, // створення екземпляру іменованого каналу
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES, 1024, 1024, 0, NULL);

        if (ConnectNamedPipe(hPipe, NULL) || GetLastError() == ERROR_PIPE_CONNECTED) // очікування підключення клієнта
            thread(HandleClient, hPipe).detach();
        else
            CloseHandle(hPipe);
    }
    return 0;
}