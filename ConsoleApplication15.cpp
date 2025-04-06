#include <windows.h>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <random>

using namespace std;

// Constants
constexpr int WINDOW_WIDTH = 650;
constexpr int WINDOW_HEIGHT = 500;
constexpr int BUTTON_COUNT = 4;
constexpr int TIMER_ID = 1;
constexpr int TIMER_DELAY = 1000;
constexpr int QUESTION_FONT_SIZE = 24;
constexpr int BUTTON_FONT_SIZE = 18;
constexpr INT_PTR BUTTON_ID_BASE = 100;
constexpr INT_PTR RESTART_BUTTON_ID = 200;
constexpr INT_PTR EXIT_BUTTON_ID = 201;


constexpr int TIMER_UPDATE_INTERVAL = 1000; // 1 секунда за обновяване
constexpr int QUIZ_TIME_LIMIT = 150000; // 150 секунди за целия тест
int remainingTime = QUIZ_TIME_LIMIT / 1000; // Времето в секунди

// Application states
enum class AppState { START_SCREEN, QUIZ_RUNNING, RESULTS_SCREEN };

// Question structure
struct Question {
    wstring questionText;
    vector<wstring> options;
    wchar_t correctOption;

    void ShuffleOptions() {
        static random_device rd;
        static mt19937 g(rd());
        wstring correct = options[correctOption - 'A'];
        shuffle(options.begin(), options.end(), g);
        auto it = find(options.begin(), options.end(), correct);
        correctOption = static_cast<wchar_t>('A' + distance(options.begin(), it));
    }
};

// Global variables
vector<Question> quiz = {
    {L"What is the size of an int in C++?", {L"2 bytes", L"4 bytes", L"8 bytes", L"Depends on Compiler"}, L'D'},
    {L"Which keyword is used to define a constant in C++?", {L"constant", L"final", L"const", L"constexpr"}, L'C'},
    {L"What is the output of 5/2 in C++ (assuming integer division)?", {L"2", L"2.5", L"3", L"None of these"}, L'A'},
    {L"Which data structure uses FIFO principle?", {L"Stack", L"Queue", L"Heap", L"Graph"}, L'B'},
    {L"What does OOP stand for in C++?",{L"Object-Oriented Programming", L"Object-Option Protocol", L"Operational Object Process", L"Oriented Object Programming"},L'A'},
    {L"Which operator is used for dynamic memory allocation in C++?",{L"malloc", L"new", L"create", L"allocate"},L'B'},
    {L"What is the correct way to declare a pure virtual function?",{L"virtual void func() = 0;", L"void virtual func() = 0;", L"void func() pure;", L"pure virtual void func();"},L'A'},
    {L"Which STL container provides fastest access to elements by index?",{L"std::list", L"std::vector", L"std::map", L"std::set"},L'B'},
    {L"What is nullptr in C++?",{L"A macro for NULL", L"A pointer literal", L"An integer zero", L"A void pointer"},L'B'},
    {L"Which C++ feature allows compile-time polymorphism?",{L"Virtual functions", L"Templates", L"Inheritance", L"Function overloading"},L'D'},
    {L"What is the scope resolution operator in C++?",{L".", L"->", L"::", L":"},L'C'},
    {L"Which header file is needed for file I/O operations?",{L"<filestream>", L"<fstream>", L"<fileio>", L"<iostream>"},L'B'}
};


AppState currentState = AppState::START_SCREEN;
int currentQuestionIndex = 0;
int score = 0;
HWND hQuestionLabel;
HWND hButtons[BUTTON_COUNT];
HWND hStartButton;
HWND hExitButton;
HWND hInfoLabel;
HWND hResultLabel;
HWND hRestartButton;
HWND hFinalExitButton;
HFONT hQuestionFont;
HFONT hButtonFont;
HFONT hResultFont;

// Color management
COLORREF g_buttonTextColor[BUTTON_COUNT];
COLORREF g_buttonBgColor[BUTTON_COUNT];
HBRUSH g_buttonBrushes[BUTTON_COUNT] = { 0 };

HWND hTimerLabel;

void ShuffleQuestions() {
    static random_device rd;
    static mt19937 g(rd());
    shuffle(quiz.begin(), quiz.end(), g);
}

HFONT CreateCustomFont(int height, int weight, const wchar_t* faceName) {
    return CreateFont(height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, faceName);
}

void UpdateButtonColors(int buttonIndex, COLORREF textColor, COLORREF bgColor) {
    g_buttonTextColor[buttonIndex] = textColor;
    g_buttonBgColor[buttonIndex] = bgColor;

    if (g_buttonBrushes[buttonIndex]) {
        DeleteObject(g_buttonBrushes[buttonIndex]);
    }
    g_buttonBrushes[buttonIndex] = CreateSolidBrush(bgColor);
    InvalidateRect(hButtons[buttonIndex], NULL, TRUE);
}

void ShowScreen(HWND hwnd, AppState newState) {
    currentState = newState;

    // Hide all elements first
    ShowWindow(hQuestionLabel, SW_HIDE);
    for (int i = 0; i < BUTTON_COUNT; ++i) {
        ShowWindow(hButtons[i], SW_HIDE);
    }
    ShowWindow(hInfoLabel, SW_HIDE);
    ShowWindow(hStartButton, SW_HIDE);
    ShowWindow(hExitButton, SW_HIDE);
    ShowWindow(hResultLabel, SW_HIDE);
    ShowWindow(hRestartButton, SW_HIDE);
    ShowWindow(hFinalExitButton, SW_HIDE);

    // Show elements based on state
    switch (currentState) {
        case AppState::START_SCREEN:
            ShowWindow(hInfoLabel, SW_SHOW);
            ShowWindow(hStartButton, SW_SHOW);
            ShowWindow(hExitButton, SW_SHOW);
            break;

        case AppState::QUIZ_RUNNING:
            ShowWindow(hQuestionLabel, SW_SHOW);
            ShowWindow(hTimerLabel, SW_SHOW);
            
            for (int i = 0; i < BUTTON_COUNT; ++i) {
                ShowWindow(hButtons[i], SW_SHOW);
            }
            break;

        case AppState::RESULTS_SCREEN:
            wstringstream resultText;
            resultText << L"The test is over!\n\n";
            resultText << L"Your result: " << score << L"/" << quiz.size() << L"\n\n";
            resultText << L"In precentage: " << (score * 100 / quiz.size()) << L"%\n\n";

            if ((score * 100 / quiz.size()) == 100) {
                resultText << L"You got a perfect score!";
            }
            else if ((score * 100 / quiz.size()) >= 75) {
                resultText << L"You need to studie a bit more!";
            }
            else {
                resultText << L"You need to studie alot more!";
            }

            SetWindowText(hResultLabel, resultText.str().c_str());
            ShowWindow(hResultLabel, SW_SHOW);
            ShowWindow(hRestartButton, SW_SHOW);
            ShowWindow(hFinalExitButton, SW_SHOW);
            ShowWindow(hTimerLabel, SW_HIDE);
            break;
        }
}

void ShowStartScreen(HWND hwnd) {
    currentState = AppState::START_SCREEN;

    ShowWindow(hQuestionLabel, SW_HIDE);
    for (int i = 0; i < BUTTON_COUNT; ++i) {
        ShowWindow(hButtons[i], SW_HIDE);
    }

    ShowWindow(hInfoLabel, SW_SHOW);
    ShowWindow(hStartButton, SW_SHOW);
    ShowWindow(hExitButton, SW_SHOW);

    wstring infoText = L"C++ Knowledge Quiz\n\n";
    infoText += L"• Contains " + to_wstring(quiz.size()) + L" questions\n";
    infoText += L"• Each question has 4 possible answers\n";
    infoText += L"• You have unlimited time to answer\n\n";
    infoText += L"Click 'Start' to begin!";

    SetWindowText(hInfoLabel, infoText.c_str());
}

void StartQuiz(HWND hwnd) {
    currentQuestionIndex = 0;
    score = 0;
    ShuffleQuestions();
    remainingTime = QUIZ_TIME_LIMIT / 1000;

    // Пуснете и двата таймера
    SetTimer(hwnd, 2, TIMER_UPDATE_INTERVAL, NULL); // Таймер за обратно броене
    SetTimer(hwnd, TIMER_ID, TIMER_DELAY, NULL);    // Таймер за преминаване

    ShowScreen(hwnd, AppState::QUIZ_RUNNING);

    // Инициализация на първия въпрос
    if (currentQuestionIndex < quiz.size()) {
        quiz[currentQuestionIndex].ShuffleOptions();
        SetWindowText(hQuestionLabel, quiz[currentQuestionIndex].questionText.c_str());
        for (int i = 0; i < BUTTON_COUNT; ++i) {
            SetWindowText(hButtons[i], quiz[currentQuestionIndex].options[i].c_str());
            UpdateButtonColors(i, RGB(0, 0, 0), RGB(240, 240, 240));
            EnableWindow(hButtons[i], TRUE);
        }
    }
}

void ShowAnswerFeedback(HWND hwnd, int selectedIndex) {
    bool isCorrect = (L'A' + selectedIndex == quiz[currentQuestionIndex].correctOption);

    if (isCorrect) {
        score++;
    }

    // Оцветяване на бутоните
    for (int i = 0; i < BUTTON_COUNT; ++i) {
        bool isRightAnswer = (L'A' + i == quiz[currentQuestionIndex].correctOption);

        if (i == selectedIndex) {
            UpdateButtonColors(i, RGB(255, 255, 255),
                isCorrect ? RGB(0, 200, 0) : RGB(200, 0, 0));
        }
        else if (isRightAnswer) {
            UpdateButtonColors(i, RGB(255, 255, 255), RGB(0, 150, 0));
        }
        else {
            UpdateButtonColors(i, RGB(100, 100, 100), RGB(240, 240, 240));
        }

        EnableWindow(hButtons[i], FALSE);
    }

    currentQuestionIndex++;
    SetTimer(hwnd, TIMER_ID, TIMER_DELAY, NULL);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            hQuestionFont = CreateCustomFont(QUESTION_FONT_SIZE, FW_BOLD, L"Arial");
            hButtonFont = CreateCustomFont(BUTTON_FONT_SIZE, FW_NORMAL, L"Arial");
            hResultFont = CreateCustomFont(28, FW_BOLD, L"Arial");

            // Info label
            hInfoLabel = CreateWindow(L"STATIC", L"",
                WS_VISIBLE | WS_CHILD | SS_CENTER,
                20, 20, WINDOW_WIDTH - 40, 300,
                hwnd, NULL, NULL, NULL);
            SendMessage(hInfoLabel, WM_SETFONT, (WPARAM)hQuestionFont, TRUE);

            // Start button
            hStartButton = CreateWindow(L"BUTTON", L"Start",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                WINDOW_WIDTH / 2 - 220, 350, 200, 50,
                hwnd, (HMENU)1, NULL, NULL);
            SendMessage(hStartButton, WM_SETFONT, (WPARAM)hButtonFont, TRUE);

            // Exit button
            hExitButton = CreateWindow(L"BUTTON", L"Exit",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                WINDOW_WIDTH / 2 + 20, 350, 200, 50,
                hwnd, (HMENU)2, NULL, NULL);
            SendMessage(hExitButton, WM_SETFONT, (WPARAM)hButtonFont, TRUE);

            // Quiz controls
            hQuestionLabel = CreateWindow(L"STATIC", L"",
                WS_CHILD | SS_CENTER,
                20, 20, WINDOW_WIDTH - 40, 100,
                hwnd, NULL, NULL, NULL);
            SendMessage(hQuestionLabel, WM_SETFONT, (WPARAM)hQuestionFont, TRUE);

            for (int i = 0; i < BUTTON_COUNT; ++i) {
                hButtons[i] = CreateWindow(L"BUTTON", L"",
                    WS_CHILD | BS_PUSHBUTTON,
                    50, 150 + (i * 70), WINDOW_WIDTH - 100, 50,
                    hwnd, (HMENU)(BUTTON_ID_BASE + i), NULL, NULL);
                SendMessage(hButtons[i], WM_SETFONT, (WPARAM)hButtonFont, TRUE);
                UpdateButtonColors(i, RGB(0, 0, 0), RGB(240, 240, 240));
            }

            // Results label
            hResultLabel = CreateWindow(L"STATIC", L"",
                WS_CHILD | SS_CENTER,
                20, 50, WINDOW_WIDTH - 40, 300,
                hwnd, NULL, NULL, NULL);
            SendMessage(hResultLabel, WM_SETFONT, (WPARAM)hResultFont, TRUE);

            // Restart button (results screen)
            hRestartButton = CreateWindow(L"BUTTON", L"Start over",
                WS_CHILD | BS_PUSHBUTTON,
                WINDOW_WIDTH / 2 - 220, 350, 200, 50,
                hwnd, (HMENU)RESTART_BUTTON_ID, NULL, NULL);
            SendMessage(hRestartButton, WM_SETFONT, (WPARAM)hButtonFont, TRUE);

            // Exit button (results screen)
            hFinalExitButton = CreateWindow(L"BUTTON", L"Exit",
                WS_CHILD | BS_PUSHBUTTON,
                WINDOW_WIDTH / 2 + 20, 350, 200, 50,
                hwnd, (HMENU)EXIT_BUTTON_ID, NULL, NULL);
            SendMessage(hFinalExitButton, WM_SETFONT, (WPARAM)hButtonFont, TRUE);

            hTimerLabel = CreateWindow(L"STATIC", L"Remaining time: ",
                WS_CHILD | SS_CENTER,
                WINDOW_WIDTH - 200, 50, 180, 30,
                hwnd, NULL, NULL, NULL);
            SendMessage(hTimerLabel, WM_SETFONT, (WPARAM)hButtonFont, TRUE);

            ShowStartScreen(hwnd);
            break;
        }

        case WM_CTLCOLORBTN: {
            for (int i = 0; i < BUTTON_COUNT; ++i) {
                if ((HWND)lParam == hButtons[i]) {
                    HDC hdc = (HDC)wParam;
                    SetTextColor(hdc, g_buttonTextColor[i]);
                    SetBkColor(hdc, g_buttonBgColor[i]);
                    return (LRESULT)g_buttonBrushes[i];
                }
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);

            switch (currentState) {
            case AppState::START_SCREEN:
                if (wmId == 1) StartQuiz(hwnd);
                else if (wmId == 2) PostQuitMessage(0);
                break;

            case AppState::QUIZ_RUNNING:
                for (int i = 0; i < BUTTON_COUNT; ++i) {
                    if (wmId == BUTTON_ID_BASE + i) {
                        ShowAnswerFeedback(hwnd, i);
                        break;
                    }
                }
                break;

            case AppState::RESULTS_SCREEN:
                if (wmId == RESTART_BUTTON_ID) {
                    ShowScreen(hwnd, AppState::START_SCREEN);
                }
                else if (wmId == EXIT_BUTTON_ID) {
                    PostQuitMessage(0);
                }
                break;
            }
            break;
        }

        case WM_TIMER:
            if (wParam == TIMER_ID) {
                // Това е таймерът за преминаване към следващия въпрос
                KillTimer(hwnd, TIMER_ID);
                if (currentQuestionIndex < quiz.size()) {
                    quiz[currentQuestionIndex].ShuffleOptions();
                    SetWindowText(hQuestionLabel, quiz[currentQuestionIndex].questionText.c_str());
                    for (int i = 0; i < BUTTON_COUNT; ++i) {
                        SetWindowText(hButtons[i], quiz[currentQuestionIndex].options[i].c_str());
                        UpdateButtonColors(i, RGB(0, 0, 0), RGB(240, 240, 240));
                        EnableWindow(hButtons[i], TRUE);
                    }
                }
                else {
                    KillTimer(hwnd, 2); // Спираме таймера за обратно броене
                    ShowScreen(hwnd, AppState::RESULTS_SCREEN);
                }
            }
            else if (wParam == 2) {
                // Това е таймерът за обратно броене
                remainingTime--;
                wstring timerText = L"Remaining time: " + to_wstring(remainingTime) + L"s";
                SetWindowText(hTimerLabel, timerText.c_str());

                if (remainingTime <= 0) {
                    KillTimer(hwnd, 2);
                    KillTimer(hwnd, TIMER_ID);
                    ShowScreen(hwnd, AppState::RESULTS_SCREEN);
                }
            }
            break;

        case WM_DESTROY:
            for (int i = 0; i < BUTTON_COUNT; ++i) {
                if (g_buttonBrushes[i]) {
                    DeleteObject(g_buttonBrushes[i]);
                }
            }
            DeleteObject(hQuestionFont);
            DeleteObject(hButtonFont);
            DeleteObject(hResultFont);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"QuizApp";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClass(&wc);

    HWND hwnd = CreateWindow(
        L"QuizApp", L"C++ Quiz",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}