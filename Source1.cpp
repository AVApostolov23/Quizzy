#include <windows.h>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <random>

using namespace std;

// Constants for better maintainability
constexpr int WINDOW_WIDTH = 500;
constexpr int WINDOW_HEIGHT = 400;
constexpr int BUTTON_COUNT = 4;
constexpr int TIMER_ID = 1;
constexpr int TIMER_DELAY = 1000; // 1 second
constexpr int QUESTION_FONT_SIZE = 24;
constexpr int BUTTON_FONT_SIZE = 18;
const INT_PTR BUTTON_ID_BASE = 100;



struct Question {
    wstring questionText;
    vector<wstring> options;
    wchar_t correctOption;
    int correctIndex;  // Запазваме оригиналния индекс на верния отговор

    void ShuffleOptions() {
        static random_device rd;
        static mt19937 g(rd());

        // Запазваме верния отговор преди разбъркване
        wstring correct = options[correctOption - 'A'];

        // Разбъркваме
        shuffle(options.begin(), options.end(), g);

        // Намираме новия индекс на верния отговор
        auto it = find(options.begin(), options.end(), correct);
        correctOption = 'A' + distance(options.begin(), it);
    }
};

vector<Question> quiz = {
    {L"What is the size of an int in C++?", {L"2 bytes", L"4 bytes", L"8 bytes", L"Depends on Compiler"}, L'D'},
    {L"Which keyword is used to define a constant in C++?", {L"constant", L"final", L"const", L"constexpr"}, L'C'},
    {L"What is the output of 5/2 in C++ (assuming integer division)?", {L"2", L"2.5", L"3", L"None of these"}, L'A'},
    {L"Which data structure uses FIFO principle?", {L"Stack", L"Queue", L"Heap", L"Graph"}, L'B'}
};

//function for shuffling
void ShuffleQuestions() {
    static std::random_device rd;
    static std::mt19937 g(rd());
    shuffle(quiz.begin(), quiz.end(), g);
}

int currentQuestionIndex = 0;
int score = 0;
HWND hQuestionLabel;
HWND hButtons[BUTTON_COUNT];
HFONT hQuestionFont;
HFONT hButtonFont;

void CreateFonts() {
    hQuestionFont = CreateFont(
        QUESTION_FONT_SIZE, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");

    hButtonFont = CreateFont(
        BUTTON_FONT_SIZE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
}

void LoadQuestion(HWND hwnd) {
    if (currentQuestionIndex >= quiz.size()) {
        wstringstream result;
        result << L"Quiz Completed!\n\nYour score: " << score << L"/" << quiz.size();
        MessageBox(hwnd, result.str().c_str(), L"Result", MB_OK | MB_ICONINFORMATION);
        PostQuitMessage(0);
        return;
    }


    // Разбъркваме отговорите за текущия въпрос
    quiz[currentQuestionIndex].ShuffleOptions();

    // Показваме въпроса и отговорите
    SetWindowText(hQuestionLabel, quiz[currentQuestionIndex].questionText.c_str());
    for (int i = 0; i < BUTTON_COUNT; ++i) {
        SetWindowText(hButtons[i], quiz[currentQuestionIndex].options[i].c_str());
        EnableWindow(hButtons[i], TRUE);  // Важно: активираме бутоните!
    }
}

void ShowAnswerFeedback(HWND hwnd, int buttonIndex, bool isCorrect) {
    if (isCorrect) {
        SetWindowText(hButtons[buttonIndex], L"✓ Correct!");
        score++;
    }
    else {
        wstring feedback = L"✗ Wrong! Correct: ";
        feedback += quiz[currentQuestionIndex].correctOption - L'A' + L'1'; // Show correct answer number
        SetWindowText(hButtons[buttonIndex], feedback.c_str());
    }

    // Disable all buttons after answer is selected
    for (int i = 0; i < BUTTON_COUNT; ++i) {
        EnableWindow(hButtons[i], FALSE);
    }

    currentQuestionIndex++;
    SetTimer(hwnd, TIMER_ID, TIMER_DELAY, NULL);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND:
        for (int i = 0; i < BUTTON_COUNT; ++i) {
            if ((HWND)lParam == hButtons[i]) {
                // Маркираме избрания отговор
                if (L'A' + i == quiz[currentQuestionIndex].correctOption) {
                    SetWindowText(hButtons[i], L"✓ Correct!");
                    score++;
                }
                else {
                    wstring feedback = L"✗ Wrong! The correct is: ";
                    feedback += quiz[currentQuestionIndex].correctOption;
                    SetWindowText(hButtons[i], feedback.c_str());
                }

                // Деактивираме само СЛЕД избор на отговор
                for (int j = 0; j < BUTTON_COUNT; ++j) {
                    EnableWindow(hButtons[j], FALSE);
                }

                currentQuestionIndex++;
                SetTimer(hwnd, TIMER_ID, TIMER_DELAY, NULL);
                break;
            }
        }
        break;

    case WM_TIMER:
        KillTimer(hwnd, TIMER_ID);
        LoadQuestion(hwnd);
        break;

    case WM_CREATE:
        CreateFonts();

        hQuestionLabel = CreateWindow(
            L"STATIC", L"",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            20, 20, WINDOW_WIDTH - 40, 60,
            hwnd, NULL, NULL, NULL);
        SendMessage(hQuestionLabel, WM_SETFONT, (WPARAM)hQuestionFont, TRUE);

        for (int i = 0; i < BUTTON_COUNT; ++i) {
            hButtons[i] = CreateWindow(
                L"BUTTON", L"",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                50, 100 + (i * 60), WINDOW_WIDTH - 100, 40,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(100 + i)), NULL, NULL);
            SendMessage(hButtons[i], WM_SETFONT, (WPARAM)hButtonFont, TRUE);
        }
        LoadQuestion(hwnd);
        break;

    case WM_DESTROY:
        DeleteObject(hQuestionFont);
        DeleteObject(hButtonFont);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    ShuffleQuestions();
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

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}