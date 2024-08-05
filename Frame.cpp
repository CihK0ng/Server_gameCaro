#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <thread>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#define BUTTON_ID_START 1000
#define GRID_SIZE 15
#define ID_EDIT_CHAT 101
#define ID_BUTTON_SEND 102

#define ID_BUTTON_BewGame 103
#define rowa 15
#define cola 15


bool isTurn = false;

HWND hwnd , hwndFindP, NewGame, Label, connectSV, FindP, sendbtn ,BoxChat;
HWND buttons[GRID_SIZE][GRID_SIZE];
SOCKET clientSocket;

void InitializeWinsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        exit(1);
    }
}

void CleanupWinsock() {
    WSACleanup();
}


SOCKET CreateServerSocket() {
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
        CleanupWinsock();
        exit(1);
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        CleanupWinsock();
        exit(1);
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        CleanupWinsock();
        exit(1);
    }

    return serverSocket;
}

SOCKET AcceptClientConnection(SOCKET serverSocket) {
    SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        CleanupWinsock();
        exit(1);
    }
    return clientSocket;
}

void disableButton(boolean isturn) {
    if (isturn == false) {

        for (int i = 0; i < rowa; i++) {
            for (int j = 0; j < cola; j++) {

                EnableWindow(buttons[i][j], FALSE);
            }
        }
    }
    else {

        for (int i = 0; i < rowa; i++) {
            for (int j = 0; j < cola; j++) {

                EnableWindow(buttons[i][j], TRUE);
            }
        }
    }
}

std::wstring GetTextBtn(HWND hwnd) {
    wchar_t buffer[2];
    GetWindowText(hwnd, buffer, 2);
    return std::wstring(buffer);
}


void WinNotification(std::wstring player) {
    MessageBox(hwnd, (player + L" win").c_str(), L"Message", MB_OK);
    
}


void CheckWin(int rows, int cols) {
    std::wstring player = GetTextBtn(buttons[rows][cols]);

    int count = 0;
    // kiem tra hang ngang
    for (int j = 0; j < cola; j++) {
        if (GetTextBtn(buttons[rows][j]) == player) {
            count++;
            if (count == 5) {
                WinNotification(player);
                
                return;
            }
        }
        else {
            count = 0;
        }
    }

    //kiemtra hang doc
    count = 0;
    for (int i = 0; i < rowa; i++) {
        if (GetTextBtn(buttons[i][cols]) == player) {
            count++;
            if (count == 5) {
                WinNotification(player);
                
                return;
            }
        }
        else {
            count = 0;
        }
    }

    // kiem tra duong cheo trai 
    int r = rows;
    int c = cols;
    while (r != 0 && c != 0) {
        r--;
        c--;
    }
    count = 0;
    while (true) {
        if (GetTextBtn(buttons[r][c]) == player) {
            count++;
            if (count == 5) {
                WinNotification(player);
                
                return;
            }
        }
        else {
            count = 0;
        }
        r++;
        c++;
        if (r == rowa || c == cola) {
            break;
        }
    }

    // kiem tra duong cheo phai 
    r = rows;
    c = cols;
    while (r != 0 && c != cola - 1) {
        r--;
        c++;
    }
    count = 0;
    while (true) {
        if (GetTextBtn(buttons[r][c]) == player) {
            count++;
            if (count == 5) {
                WinNotification(player);
               
                return;
            }
        }
        else {
            count = 0;
        }

        r++;
        c--;
        if (r == rowa || c == -1) {
            break;
        }
    }

}

boolean CheckBtnempty(HWND Btn) {
    int a = GetWindowTextLength(Btn);

    if (a == 1) {
        return true;
    }
    else {
        return false;
    }

}



static void restart() {
    for (int j = 0; j < rowa; j++) {
        for (int i = 0; i < rowa; i++) {
            SetWindowText(buttons[i][j], L"");
        }
    }
}




// chuc nang 


void NetworkThread(SOCKET clientSocket) {
    char buffer[512];
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            int row, col;
           
            isTurn = true;
            disableButton(isTurn);
          
            sscanf_s(buffer, "%d,%d", &row, &col);
            
            SendMessage(hwnd, WM_USER + 1, row, col);

            SendMessage(BoxChat, WM_SETTEXT, 0, (LPARAM)buffer);
            CheckWin(row, col);
          
        }
    }
}





LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        for (int i = 0; i < GRID_SIZE; ++i) {
            for (int j = 0; j < GRID_SIZE; ++j) {
                buttons[i][j] = CreateWindow(
                    L"BUTTON", L"",
                    WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                    50 + j * 30, 50 + i * 30, 30, 30,
                    hwnd, (HMENU)(intptr_t)(BUTTON_ID_START + i * GRID_SIZE + j),
                    (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
            }
        }

        Label = CreateWindowW(
            L"STATIC",
            L"Trang thai: ",
            WS_VISIBLE | WS_CHILD,
            550, 80, 300, 25,
            hwnd,
            NULL,
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL
        );

        connectSV = CreateWindowEx(
            0,
            L"BUTTON",
            L"Ket Noi",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            550, 40, 80, 25,
            hwnd,
            (HMENU)(ID_BUTTON_BewGame),
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL
        );

        NewGame = CreateWindowW(
            L"BUTTON",
            L"New Game",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            5, 5, 80, 20,
            hwnd,
            (HMENU)(ID_BUTTON_BewGame),
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL
        );

        FindP = CreateWindowW(
            L"BUTTON",
            L"Find Player",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            750, 40, 80, 25,
            hwnd,
            (HMENU)(60), // 
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
            NULL
            );

        BoxChat = CreateWindowEx(
            0,
            L"EDIT",
            0,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
            580, 100, 300, 200,
            hwnd,
            (HMENU)ID_EDIT_CHAT,
            GetModuleHandle(NULL), NULL
        );

    
        sendbtn  = CreateWindow(
            L"BUTTON",
            L"Send",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            580, 300, 80, 20,
            hwnd,
            (HMENU)ID_BUTTON_SEND,
            GetModuleHandle(NULL), NULL
        );

        break;
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) {
            int buttonId = LOWORD(wParam) - BUTTON_ID_START;
            int row = buttonId / GRID_SIZE;
            int col = buttonId % GRID_SIZE;
            SetWindowText(buttons[row][col], L"X");

            char buffer[512];
            sprintf_s(buffer, "%d,%d", row, col);
            send(clientSocket, buffer, (int)strlen(buffer), 0);

           
                CheckWin(row, col);

                isTurn = false;
                disableButton(isTurn);
            

        }
        else if (LOWORD(wParam) == ID_BUTTON_SEND)
        {
            wchar_t mess[512];
            int len = GetWindowTextLength(BoxChat);
            GetWindowText(BoxChat, mess, len + 1);

            char message[512];
            int messageLen = WideCharToMultiByte(CP_ACP, 0, mess, -1, message, sizeof(message), NULL, NULL);

            // Gửi tin nhắn đến client
             send(clientSocket, message, messageLen - 1, 0); // loại bỏ  null 

     

            // Xóa ô văn bản sau khi gửi tin nhắn
            SetWindowText(BoxChat, L"");
        }
        else if (LOWORD(wParam) == ID_BUTTON_BewGame) {
            restart();
        }
        break;
    case WM_USER + 1: {
        int row = (int)wParam;
        int col = (int)lParam;
        SetWindowText(buttons[row][col], L"O");
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    InitializeWinsock();
    SOCKET serverSocket = CreateServerSocket();
    std::thread networkThread;
    clientSocket = AcceptClientConnection(serverSocket);

    static TCHAR CLASS_NAME[] = L"CaroWindowClass";
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

     hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Caro Game Server",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1000, 650,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    std::cout << "Waiting for client connection..." << std::endl;
    
    std::cout << "Client connected." << std::endl;

    networkThread = std::thread(NetworkThread, clientSocket);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    networkThread.join();
    closesocket(clientSocket);
    closesocket(serverSocket);
    CleanupWinsock();

    return 0;
}
