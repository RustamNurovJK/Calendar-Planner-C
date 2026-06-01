#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "msimg32.lib")

#define MAX_TASKS 100
#define MAX_TITLE 200
#define MAX_DESC 500
#define DAYS_IN_WEEK 7
#define WEEKS_TO_SHOW 6
#define TIMER_ID 1
#define TIMER_ID_REMINDER 2
#define TIMER_INTERVAL 1000
#define REMINDER_DELAY 500

typedef struct {
    int day, month, year;
} Date;

typedef struct {
    char title[MAX_TITLE];
    char description[MAX_DESC];
    int priority;
    int completed;
    int hasTime;
    int hour;
    int minute;
    int taskDay;
    int taskMonth;
    int taskYear;
} Task;

Task tasks[366][MAX_TASKS];
int taskCount[366] = { 0 };
int currentMonth, currentYear;
int selectedDay = 0;
int selectedTaskIndex = -1;
HWND g_mainHwnd = NULL;
HWND g_hMonthText = NULL;
HWND g_hPrevBtn = NULL;
HWND g_hNextBtn = NULL;
HWND g_hClockText = NULL;

HBITMAP g_hBackgroundBitmap = NULL;
Date currentDate;

int isAnimating = 0;
int animationStep = 0;
int animationDirection = 0;

const char* monthNames[] = { "January", "February", "March", "April", "May", "June",
                            "July", "August", "September", "October", "November", "December" };

int getDaysInMonth(int month, int year);
int getDayOfWeek(int day, int month, int year);
Date getCurrentDate();
int getDayOfYear(int day, int month, int year);
int compareDates(Date d1, Date d2);
int isPastDate(int day, int month, int year);
int isToday(Date targetDate);
int isTomorrow(Date targetDate);
int isOverdue(int day, int month, int year, int hasTime, int hour, int minute);
void removeOverdueTasks();
void showTaskDescription(int day, int month, int year, int taskIndex);
void checkOverdueReminders();
void checkReminders();
void getPrioritiesForDay(int day, int month, int year, int* hasLow, int* hasMid, int* hasHigh);
void addTask(int day, int month, int year, const char* title, const char* desc, int priority, int hasTime, int hour, int minute);
void deleteTask(int day, int month, int year, int taskIndex);
void completeTask(int day, int month, int year, int taskIndex);
void saveTasksToFile();
void loadTasksFromFile();
void drawCalendar(HDC hdc, int x, int y, int width, int height);
void drawTaskPanel(HDC hdc, int x, int y, int width, int height);
void updateMonthDisplay();
void updateClock();
void goPreviousMonth();
void goNextMonth();
void animateMonthSwitch(int direction);
void loadBackgroundImage();
void drawBackground(HDC hdc, int width, int height);
void playStartSound();
void playExitSound();
void playAddTaskSound();
void playDeleteTaskSound();
void playCompleteTaskSound();
void playPageTurnSound();
void playPageTurnEndSound();
void playDaySelectSound();
void playReminderSound();
void playOverdueSound();

LRESULT CALLBACK AddTaskDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void playStartSound() { Beep(987, 150); Sleep(100); Beep(1318, 200); }
void playExitSound() { Beep(1046, 180); Sleep(80); Beep(987, 180); Sleep(80); Beep(880, 250); }
void playAddTaskSound() { Beep(523, 100); Sleep(80); Beep(659, 100); Sleep(80); Beep(784, 150); }
void playDeleteTaskSound() { Beep(440, 80); Sleep(50); Beep(392, 100); }
void playCompleteTaskSound() { Beep(1046, 80); Sleep(60); Beep(1318, 80); Sleep(60); Beep(1568, 120); }
void playPageTurnSound() { Beep(1200, 15); Sleep(10); Beep(1100, 15); Sleep(10); Beep(1000, 15); Sleep(10); Beep(900, 15); }
void playPageTurnEndSound() { Beep(800, 40); Sleep(20); Beep(700, 30); }
void playDaySelectSound() { Beep(800, 30); }
void playReminderSound() { Beep(1000, 150); Sleep(100); Beep(1200, 150); Sleep(100); Beep(1000, 200); }
void playOverdueSound() { Beep(600, 200); Sleep(150); Beep(500, 300); }

int getDaysInMonth(int month, int year) {
    if (month == 2) return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) ? 29 : 28;
    if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
    return 31;
}

int getDayOfWeek(int day, int month, int year) {
    if (month < 3) { month += 12; year--; }
    int K = year % 100, J = year / 100;
    int h = (day + (13 * (month + 1)) / 5 + K + K / 4 + J / 4 + 5 * J) % 7;
    if (h == 0) return 5;
    if (h == 1) return 6;
    if (h == 2) return 0;
    if (h == 3) return 1;
    if (h == 4) return 2;
    if (h == 5) return 3;
    if (h == 6) return 4;
    return 0;
}

Date getCurrentDate() {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    Date d = { tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900 };
    return d;
}

int getDayOfYear(int day, int month, int year) {
    int daysInMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) daysInMonth[1] = 29;
    int dayOfYear = 0;
    for (int i = 0; i < month - 1; i++) dayOfYear += daysInMonth[i];
    return dayOfYear + day - 1;
}

Date getDateFromDayIndex(int dayIdx, int year) {
    Date result = { 1, 1, year };
    int remaining = dayIdx + 1;
    for (result.month = 1; result.month <= 12; result.month++) {
        int daysInMonth = getDaysInMonth(result.month, result.year);
        if (remaining <= daysInMonth) {
            result.day = remaining;
            break;
        }
        remaining -= daysInMonth;
    }
    return result;
}

int compareDates(Date d1, Date d2) {
    if (d1.year != d2.year) return d1.year > d2.year ? 1 : -1;
    if (d1.month != d2.month) return d1.month > d2.month ? 1 : -1;
    if (d1.day != d2.day) return d1.day > d2.day ? 1 : -1;
    return 0;
}

int isPastDate(int day, int month, int year) {
    Date targetDate = { day, month, year };
    Date today = getCurrentDate();
    return compareDates(targetDate, today) < 0;
}

int isToday(Date targetDate) {
    Date today = getCurrentDate();
    return (targetDate.day == today.day && targetDate.month == today.month && targetDate.year == today.year);
}

int isTomorrow(Date targetDate) {
    Date today = getCurrentDate();
    Date tomorrow = today;
    tomorrow.day++;
    if (tomorrow.day > getDaysInMonth(tomorrow.month, tomorrow.year)) {
        tomorrow.day = 1; tomorrow.month++;
        if (tomorrow.month > 12) { tomorrow.month = 1; tomorrow.year++; }
    }
    return (targetDate.day == tomorrow.day && targetDate.month == tomorrow.month && targetDate.year == tomorrow.year);
}

int isOverdue(int day, int month, int year, int hasTime, int hour, int minute) {
    Date targetDate = { day, month, year };
    Date today = getCurrentDate();
    int dateCompare = compareDates(targetDate, today);
    if (dateCompare < 0) return 1;
    if (dateCompare == 0 && hasTime) {
        time_t now = time(NULL);
        struct tm* tm = localtime(&now);
        int currentHour = tm->tm_hour;
        int currentMinute = tm->tm_min;
        if (currentHour > hour || (currentHour == hour && currentMinute > minute)) return 1;
    }
    return 0;
}

void removeOverdueTasks() {
    int removedCount = 0;
    for (int dayIdx = 0; dayIdx < 366; dayIdx++) {
        if (taskCount[dayIdx] == 0) continue;
        for (int i = taskCount[dayIdx] - 1; i >= 0; i--) {
            if (!tasks[dayIdx][i].completed) {
                if (isOverdue(tasks[dayIdx][i].taskDay, tasks[dayIdx][i].taskMonth,
                    tasks[dayIdx][i].taskYear, tasks[dayIdx][i].hasTime,
                    tasks[dayIdx][i].hour, tasks[dayIdx][i].minute)) {
                    for (int j = i; j < taskCount[dayIdx] - 1; j++) tasks[dayIdx][j] = tasks[dayIdx][j + 1];
                    taskCount[dayIdx]--;
                    removedCount++;
                }
            }
        }
    }
    if (removedCount > 0) { saveTasksToFile(); InvalidateRect(g_mainHwnd, NULL, TRUE); }
}

// Функция для показа окна с описанием задачи
void showTaskDescription(int day, int month, int year, int taskIndex) {
    int dayIndex = getDayOfYear(day, month, year);
    if (taskIndex >= 0 && taskIndex < taskCount[dayIndex]) {
        char message[1500];
        char priorityStr[30];
        switch (tasks[dayIndex][taskIndex].priority) {
        case 1: strcpy(priorityStr, "LOW"); break;
        case 2: strcpy(priorityStr, "MEDIUM"); break;
        case 3: strcpy(priorityStr, "HIGH"); break;
        default: strcpy(priorityStr, "UNKNOWN");
        }

        char timeStr[50] = "";
        if (tasks[dayIndex][taskIndex].hasTime) {
            sprintf(timeStr, "\nTime: %02d:%02d", tasks[dayIndex][taskIndex].hour, tasks[dayIndex][taskIndex].minute);
        }

        char overdueStr[50] = "";
        if (isOverdue(day, month, year, tasks[dayIndex][taskIndex].hasTime, tasks[dayIndex][taskIndex].hour, tasks[dayIndex][taskIndex].minute)) {
            strcpy(overdueStr, "\n[OVERDUE]");
        }

        sprintf(message,
            "========================================\n"
            "           TASK DETAILS\n"
            "========================================\n\n"
            "Title: %s\n"
            "Priority: %s%s%s\n\n"
            "Date: %02d.%02d.%04d\n\n"
            "Description:\n%s\n"
            "========================================",
            tasks[dayIndex][taskIndex].title,
            priorityStr,
            timeStr,
            overdueStr,
            day, month, year,
            tasks[dayIndex][taskIndex].description);

        MessageBoxA(g_mainHwnd, message, "Task Description", MB_OK | MB_ICONINFORMATION);
    }
}

void checkOverdueReminders() {
    Date today = getCurrentDate();
    int overdueCount = 0;
    char overdueText[3000] = "";
    char tempText[300] = "";

    for (int dayIdx = 0; dayIdx < 366; dayIdx++) {
        if (taskCount[dayIdx] == 0) continue;
        int remaining = dayIdx + 1;
        Date taskDate;
        taskDate.year = today.year;
        for (taskDate.month = 1; taskDate.month <= 12; taskDate.month++) {
            int daysInMonth = getDaysInMonth(taskDate.month, taskDate.year);
            if (remaining <= daysInMonth) { taskDate.day = remaining; break; }
            remaining -= daysInMonth;
        }

        for (int i = 0; i < taskCount[dayIdx]; i++) {
            if (!tasks[dayIdx][i].completed) {
                if (isOverdue(tasks[dayIdx][i].taskDay, tasks[dayIdx][i].taskMonth,
                    tasks[dayIdx][i].taskYear, tasks[dayIdx][i].hasTime,
                    tasks[dayIdx][i].hour, tasks[dayIdx][i].minute)) {
                    overdueCount++;

                    char priorityStr[20];
                    switch (tasks[dayIdx][i].priority) {
                    case 1: strcpy(priorityStr, "[LOW]"); break;
                    case 2: strcpy(priorityStr, "[MEDIUM]"); break;
                    case 3: strcpy(priorityStr, "[HIGH]"); break;
                    default: strcpy(priorityStr, "");
                    }

                    char timeStr[30] = "";
                    if (tasks[dayIdx][i].hasTime) {
                        sprintf(timeStr, " [%02d:%02d]", tasks[dayIdx][i].hour, tasks[dayIdx][i].minute);
                    }

                    char dateStr[30];
                    sprintf(dateStr, " (%02d.%02d.%04d)", tasks[dayIdx][i].taskDay,
                        tasks[dayIdx][i].taskMonth, tasks[dayIdx][i].taskYear);

                    sprintf(tempText, "  * %s %s%s%s\n", priorityStr, tasks[dayIdx][i].title, timeStr, dateStr);
                    strcat(overdueText, tempText);
                }
            }
        }
    }

    if (overdueCount > 0) {
        playOverdueSound();
        char message[3500];
        sprintf(message, "========================================\n"
            "     OVERDUE TASKS REMINDER!\n"
            "========================================\n\n"
            "The following tasks are overdue:\n\n"
            "%s\n"
            "========================================\n"
            "Total overdue tasks: %d\n"
            "========================================\n"
            "Please complete or delete them!",
            overdueText, overdueCount);
        MessageBoxA(g_mainHwnd, message, "OVERDUE TASKS", MB_OK | MB_ICONWARNING);
    }
}

void checkReminders() {
    Date today = getCurrentDate();
    int reminderCount = 0;
    char reminderText[5000] = "";
    char tempText[500] = "";
    int hasToday = 0, hasTomorrow = 0;

    for (int dayIdx = 0; dayIdx < 366; dayIdx++) {
        if (taskCount[dayIdx] == 0) continue;
        int remaining = dayIdx + 1;
        Date taskDate;
        taskDate.year = today.year;
        for (taskDate.month = 1; taskDate.month <= 12; taskDate.month++) {
            int daysInMonth = getDaysInMonth(taskDate.month, taskDate.year);
            if (remaining <= daysInMonth) { taskDate.day = remaining; break; }
            remaining -= daysInMonth;
        }

        int isTaskToday = isToday(taskDate);
        int isTaskTomorrow = isTomorrow(taskDate);

        if (isTaskToday || isTaskTomorrow) {
            for (int i = 0; i < taskCount[dayIdx]; i++) {
                if (!tasks[dayIdx][i].completed) {
                    reminderCount++;
                    char priorityStr[20];
                    switch (tasks[dayIdx][i].priority) {
                    case 1: strcpy(priorityStr, "[LOW]"); break;
                    case 2: strcpy(priorityStr, "[MEDIUM]"); break;
                    case 3: strcpy(priorityStr, "[HIGH]"); break;
                    default: strcpy(priorityStr, "");
                    }
                    char timeStr[30] = "";
                    if (tasks[dayIdx][i].hasTime) sprintf(timeStr, " [%02d:%02d]", tasks[dayIdx][i].hour, tasks[dayIdx][i].minute);

                    if (tasks[dayIdx][i].description[0] != '\0' && strlen(tasks[dayIdx][i].description) > 0) {
                        if (isTaskToday) { hasToday = 1; sprintf(tempText, "  * %s %s%s\n    > %s\n\n", priorityStr, tasks[dayIdx][i].title, timeStr, tasks[dayIdx][i].description); }
                        else { hasTomorrow = 1; sprintf(tempText, "  * %s %s%s\n    > %s\n\n", priorityStr, tasks[dayIdx][i].title, timeStr, tasks[dayIdx][i].description); }
                    }
                    else {
                        if (isTaskToday) { hasToday = 1; sprintf(tempText, "  * %s %s%s\n\n", priorityStr, tasks[dayIdx][i].title, timeStr); }
                        else { hasTomorrow = 1; sprintf(tempText, "  * %s %s%s\n\n", priorityStr, tasks[dayIdx][i].title, timeStr); }
                    }
                    strcat(reminderText, tempText);
                }
            }
        }
    }

    if (reminderCount > 0) {
        playReminderSound();
        char message[8000], dateHeader[200] = "";
        if (hasToday && hasTomorrow) {
            Date tomorrow = today;
            tomorrow.day++;
            if (tomorrow.day > getDaysInMonth(tomorrow.month, tomorrow.year)) { tomorrow.day = 1; tomorrow.month++; if (tomorrow.month > 12) { tomorrow.month = 1; tomorrow.year++; } }
            sprintf(dateHeader, "TODAY (%02d.%02d.%04d) and TOMORROW (%02d.%02d.%04d)", today.day, today.month, today.year, tomorrow.day, tomorrow.month, tomorrow.year);
        }
        else if (hasToday) {
            sprintf(dateHeader, "TODAY (%02d.%02d.%04d)", today.day, today.month, today.year);
        }
        else {
            Date tomorrow = today;
            tomorrow.day++;
            if (tomorrow.day > getDaysInMonth(tomorrow.month, tomorrow.year)) { tomorrow.day = 1; tomorrow.month++; if (tomorrow.month > 12) { tomorrow.month = 1; tomorrow.year++; } }
            sprintf(dateHeader, "TOMORROW (%02d.%02d.%04d)", tomorrow.day, tomorrow.month, tomorrow.year);
        }
        sprintf(message, "========================================\n          REMINDER!\n========================================\n\nYou have unfinished tasks for %s:\n\n%s========================================\nTotal tasks: %d\n========================================\nDon't forget to complete them on time!", dateHeader, reminderText, reminderCount);
        MessageBoxA(g_mainHwnd, message, "REMINDER", MB_OK | MB_ICONINFORMATION);
    }
}

void getPrioritiesForDay(int day, int month, int year, int* hasLow, int* hasMid, int* hasHigh) {
    int dayIndex = getDayOfYear(day, month, year);
    *hasLow = *hasMid = *hasHigh = 0;
    for (int i = 0; i < taskCount[dayIndex]; i++) {
        if (tasks[dayIndex][i].priority == 1) *hasLow = 1;
        else if (tasks[dayIndex][i].priority == 2) *hasMid = 1;
        else if (tasks[dayIndex][i].priority == 3) *hasHigh = 1;
    }
}

void saveTasksToFile() {
    FILE* f = fopen("tasks.dat", "wb");
    if (!f) return;
    fwrite(taskCount, sizeof(int), 366, f);
    for (int i = 0; i < 366; i++) {
        fwrite(tasks[i], sizeof(Task), taskCount[i], f);
    }
    fclose(f);
}

void loadTasksFromFile() {
    FILE* f = fopen("tasks.dat", "rb");
    if (!f) { for (int i = 0; i < 366; i++) taskCount[i] = 0; return; }
    fread(taskCount, sizeof(int), 366, f);
    for (int i = 0; i < 366; i++) {
        fread(tasks[i], sizeof(Task), taskCount[i], f);
    }
    fclose(f);
}

void addTask(int day, int month, int year, const char* title, const char* desc, int priority, int hasTime, int hour, int minute) {
    if (isPastDate(day, month, year)) {
        MessageBoxA(g_mainHwnd, "Cannot add task to past date! You can only plan for today and future days.", "Error", MB_OK);
        return;
    }
    int dayIndex = getDayOfYear(day, month, year);
    if (taskCount[dayIndex] < MAX_TASKS) {
        strcpy(tasks[dayIndex][taskCount[dayIndex]].title, title);
        strcpy(tasks[dayIndex][taskCount[dayIndex]].description, desc);
        tasks[dayIndex][taskCount[dayIndex]].priority = priority;
        tasks[dayIndex][taskCount[dayIndex]].completed = 0;
        tasks[dayIndex][taskCount[dayIndex]].hasTime = hasTime;
        tasks[dayIndex][taskCount[dayIndex]].hour = hour;
        tasks[dayIndex][taskCount[dayIndex]].minute = minute;
        tasks[dayIndex][taskCount[dayIndex]].taskDay = day;
        tasks[dayIndex][taskCount[dayIndex]].taskMonth = month;
        tasks[dayIndex][taskCount[dayIndex]].taskYear = year;
        taskCount[dayIndex]++;
        saveTasksToFile();
        playAddTaskSound();
        checkReminders();
    }
}

void deleteTask(int day, int month, int year, int taskIndex) {
    int dayIndex = getDayOfYear(day, month, year);
    if (taskIndex >= 0 && taskIndex < taskCount[dayIndex]) {
        for (int i = taskIndex; i < taskCount[dayIndex] - 1; i++) tasks[dayIndex][i] = tasks[dayIndex][i + 1];
        taskCount[dayIndex]--;
        saveTasksToFile();
        playDeleteTaskSound();
    }
}

void completeTask(int day, int month, int year, int taskIndex) {
    int dayIndex = getDayOfYear(day, month, year);
    if (taskIndex >= 0 && taskIndex < taskCount[dayIndex]) {
        playCompleteTaskSound();
        for (int i = taskIndex; i < taskCount[dayIndex] - 1; i++) tasks[dayIndex][i] = tasks[dayIndex][i + 1];
        taskCount[dayIndex]--;
        saveTasksToFile();
    }
}

void updateClock() {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", tm_info);
    SetWindowTextA(g_hClockText, timeStr);
}

void updateMonthDisplay() {
    char buf[100];
    sprintf(buf, "%s %d", monthNames[currentMonth - 1], currentYear);
    SetWindowTextA(g_hMonthText, buf);
    loadBackgroundImage();
    InvalidateRect(g_mainHwnd, NULL, TRUE);
}

void goPreviousMonth() {
    if (isAnimating) return;
    animateMonthSwitch(0);
    currentMonth--;
    if (currentMonth < 1) { currentMonth = 12; currentYear--; }
    int maxDay = getDaysInMonth(currentMonth, currentYear);
    if (selectedDay > maxDay) selectedDay = maxDay;
    selectedTaskIndex = -1;
    updateMonthDisplay();
}

void goNextMonth() {
    if (isAnimating) return;
    animateMonthSwitch(1);
    currentMonth++;
    if (currentMonth > 12) { currentMonth = 1; currentYear++; }
    int maxDay = getDaysInMonth(currentMonth, currentYear);
    if (selectedDay > maxDay) selectedDay = maxDay;
    selectedTaskIndex = -1;
    updateMonthDisplay();
}

void loadBackgroundImage() {
    if (g_hBackgroundBitmap) {
        DeleteObject(g_hBackgroundBitmap);
        g_hBackgroundBitmap = NULL;
    }
    char filePath[MAX_PATH];
    sprintf(filePath, "backgrounds\\%s.bmp", monthNames[currentMonth - 1]);
    g_hBackgroundBitmap = (HBITMAP)LoadImageA(NULL, filePath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!g_hBackgroundBitmap) {
        sprintf(filePath, "..\\backgrounds\\%s.bmp", monthNames[currentMonth - 1]);
        g_hBackgroundBitmap = (HBITMAP)LoadImageA(NULL, filePath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    }
}

void drawBackground(HDC hdc, int width, int height) {
    if (g_hBackgroundBitmap) {
        HDC hdcMem = CreateCompatibleDC(hdc);
        SelectObject(hdcMem, g_hBackgroundBitmap);
        BITMAP bm; GetObject(g_hBackgroundBitmap, sizeof(BITMAP), &bm);
        StretchBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
        DeleteDC(hdcMem);
    }
    else {
        COLORREF startColor, endColor;
        if (currentMonth == 12 || currentMonth == 1 || currentMonth == 2) { startColor = RGB(200, 220, 255); endColor = RGB(150, 180, 230); }
        else if (currentMonth >= 3 && currentMonth <= 5) { startColor = RGB(200, 240, 200); endColor = RGB(150, 210, 150); }
        else if (currentMonth >= 6 && currentMonth <= 8) { startColor = RGB(255, 240, 180); endColor = RGB(200, 230, 150); }
        else { startColor = RGB(255, 220, 180); endColor = RGB(230, 180, 120); }
        for (int i = 0; i < height; i++) {
            int r = (GetRValue(startColor) * (height - i) + GetRValue(endColor) * i) / height;
            int g = (GetGValue(startColor) * (height - i) + GetGValue(endColor) * i) / height;
            int b = (GetBValue(startColor) * (height - i) + GetBValue(endColor) * i) / height;
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(r, g, b));
            SelectObject(hdc, hPen);
            MoveToEx(hdc, 0, i, NULL); LineTo(hdc, width, i);
            DeleteObject(hPen);
        }
    }
}

void animateMonthSwitch(int direction) {
    if (isAnimating) return;
    isAnimating = 1;
    playPageTurnSound();
    for (int step = 1; step <= 8; step++) {
        HDC hdc = GetDC(g_mainHwnd); RECT rect; GetClientRect(g_mainHwnd, &rect);
        int brightness = (step <= 4) ? (200 - step * 20) : (120 + (step - 4) * 25);
        if (brightness < 80) brightness = 80; if (brightness > 255) brightness = 255;
        BLENDFUNCTION blend = { AC_SRC_OVER, 0, (BYTE)(255 - brightness), 0 };
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
        SelectObject(hdcMem, hbmMem);
        BitBlt(hdcMem, 0, 0, rect.right, rect.bottom, hdc, 0, 0, SRCCOPY);
        HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdcMem, &rect, whiteBrush); DeleteObject(whiteBrush);
        AlphaBlend(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, rect.right, rect.bottom, blend);
        DeleteDC(hdcMem); DeleteObject(hbmMem);
        ReleaseDC(g_mainHwnd, hdc);
        if (step == 4) { Beep(950, 20); Sleep(5); Beep(850, 20); }
        Sleep(12);
    }
    playPageTurnEndSound();
    isAnimating = 0;
    InvalidateRect(g_mainHwnd, NULL, TRUE);
    UpdateWindow(g_mainHwnd);
}

void drawCalendar(HDC hdc, int x, int y, int width, int height) {
    HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
    HBRUSH lightGrayBrush = CreateSolidBrush(RGB(240, 240, 240));
    HBRUSH selectedBrush = CreateSolidBrush(RGB(200, 220, 255));
    HBRUSH pastDateBrush = CreateSolidBrush(RGB(230, 230, 230));
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));

    int cellW = width / DAYS_IN_WEEK, cellH = height / WEEKS_TO_SHOW;
    const char* weekDays[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
    for (int i = 0; i < DAYS_IN_WEEK; i++) {
        RECT rect = { x + i * cellW, y, x + (i + 1) * cellW, y + 30 };
        FillRect(hdc, &rect, lightGrayBrush);
        DrawTextA(hdc, weekDays[i], -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    int daysInMonth = getDaysInMonth(currentMonth, currentYear);
    int firstDayOfWeek = getDayOfWeek(1, currentMonth, currentYear);

    for (int day = 1; day <= daysInMonth; day++) {
        int row = (firstDayOfWeek + day - 1) / DAYS_IN_WEEK;
        int col = (firstDayOfWeek + day - 1) % DAYS_IN_WEEK;
        int cellX = x + col * cellW, cellY = y + 30 + row * cellH;
        int isPast = isPastDate(day, currentMonth, currentYear);
        int isSelected = (day == selectedDay);

        HBRUSH bgBrush = whiteBrush;
        if (isPast) bgBrush = pastDateBrush;
        else if (isSelected) bgBrush = selectedBrush;

        RECT cellRect = { cellX, cellY, cellX + cellW, cellY + cellH };
        FillRect(hdc, &cellRect, bgBrush);
        Rectangle(hdc, cellX, cellY, cellX + cellW, cellY + cellH);

        char dayStr[4]; sprintf(dayStr, "%d", day);
        RECT textRect = { cellX, cellY + 5, cellX + cellW, cellY + 25 };
        if (isPast) SetTextColor(hdc, RGB(150, 150, 150));
        else SetTextColor(hdc, RGB(0, 0, 0));
        DrawTextA(hdc, dayStr, -1, &textRect, DT_CENTER | DT_TOP);
        SetTextColor(hdc, RGB(0, 0, 0));

        int hasLow, hasMid, hasHigh;
        getPrioritiesForDay(day, currentMonth, currentYear, &hasLow, &hasMid, &hasHigh);

        int circleX = cellX + 5, circleY = cellY + 3, circleSize = 10, spacing = 12;
        HPEN nullPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
        SelectObject(hdc, nullPen);
        if (hasLow && !isPast) { HBRUSH lowBrush = CreateSolidBrush(RGB(76, 175, 80)); SelectObject(hdc, lowBrush); Ellipse(hdc, circleX, circleY, circleX + circleSize, circleY + circleSize); DeleteObject(lowBrush); circleX += spacing; }
        if (hasMid && !isPast) { HBRUSH midBrush = CreateSolidBrush(RGB(255, 193, 7)); SelectObject(hdc, midBrush); Ellipse(hdc, circleX, circleY, circleX + circleSize, circleY + circleSize); DeleteObject(midBrush); circleX += spacing; }
        if (hasHigh && !isPast) { HBRUSH highBrush = CreateSolidBrush(RGB(244, 67, 54)); SelectObject(hdc, highBrush); Ellipse(hdc, circleX, circleY, circleX + circleSize, circleY + circleSize); DeleteObject(highBrush); }
        DeleteObject(nullPen);

        if (isSelected && !isPast) {
            HPEN redPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
            SelectObject(hdc, redPen);
            HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
            SelectObject(hdc, nullBrush);
            Ellipse(hdc, cellX + 8, cellY + 3, cellX + cellW - 8, cellY + 28);
            DeleteObject(redPen);
        }
    }
    DeleteObject(whiteBrush); DeleteObject(lightGrayBrush); DeleteObject(selectedBrush); DeleteObject(pastDateBrush); DeleteObject(borderPen);
}

void drawTaskPanel(HDC hdc, int x, int y, int width, int height) {
    HBRUSH bgBrush = CreateSolidBrush(RGB(255, 255, 255));
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    RECT bgRect = { x - 5, y - 5, x + width + 5, y + height + 5 };
    FillRect(hdc, &bgRect, bgBrush);
    Rectangle(hdc, x - 5, y - 5, x + width + 5, y + height + 5);
    RECT panelRect = { x, y, x + width, y + height };
    FillRect(hdc, &panelRect, bgBrush);
    Rectangle(hdc, x, y, x + width, y + height);

    char title[100];
    sprintf(title, "Tasks for %d.%d.%d", selectedDay, currentMonth, currentYear);
    SetBkMode(hdc, TRANSPARENT);
    int isSelectedPast = isPastDate(selectedDay, currentMonth, currentYear);
    if (isSelectedPast) SetTextColor(hdc, RGB(150, 150, 150));
    TextOutA(hdc, x + 10, y + 10, title, strlen(title));
    SetTextColor(hdc, RGB(0, 0, 0));

    int dayIndex = getDayOfYear(selectedDay, currentMonth, currentYear);
    int textY = y + 40;

    if (taskCount[dayIndex] == 0) {
        TextOutA(hdc, x + 15, textY, "No tasks for this day", 21);
    }
    else {
        for (int i = 0; i < taskCount[dayIndex] && i < 15; i++) {
            char taskText[MAX_TITLE + 80];
            COLORREF textColor;

            int isTaskOverdue = isOverdue(selectedDay, currentMonth, currentYear,
                tasks[dayIndex][i].hasTime,
                tasks[dayIndex][i].hour, tasks[dayIndex][i].minute);

            if (isTaskOverdue && !tasks[dayIndex][i].completed) {
                textColor = RGB(128, 128, 128);
            }
            else {
                switch (tasks[dayIndex][i].priority) {
                case 1: textColor = RGB(76, 175, 80); break;
                case 2: textColor = RGB(255, 152, 0); break;
                case 3: textColor = RGB(244, 67, 54); break;
                default: textColor = RGB(0, 0, 0);
                }
            }

            char prefix[30] = "";
            if (isTaskOverdue && !tasks[dayIndex][i].completed) {
                strcpy(prefix, "[OVERDUE] ");
            }
            else {
                if (tasks[dayIndex][i].priority == 1) strcpy(prefix, "[LOW] ");
                else if (tasks[dayIndex][i].priority == 2) strcpy(prefix, "[MEDIUM] ");
                else if (tasks[dayIndex][i].priority == 3) strcpy(prefix, "[HIGH] ");
            }

            char timeStr[20] = "";
            if (tasks[dayIndex][i].hasTime) {
                sprintf(timeStr, " [%02d:%02d]", tasks[dayIndex][i].hour, tasks[dayIndex][i].minute);
            }

            // Стрелочка ▼ если есть описание
            char arrow[10] = "";
            if (tasks[dayIndex][i].description[0] != '\0' && strlen(tasks[dayIndex][i].description) > 0) {
                strcpy(arrow, "▼ ");
            }

            sprintf(taskText, "%s%s%s%s", arrow, prefix, tasks[dayIndex][i].title, timeStr);

            if (i == selectedTaskIndex) {
                RECT rect = { x + 5, textY - 2, x + width - 5, textY + 20 };
                HBRUSH selectBrush = CreateSolidBrush(RGB(200, 220, 255));
                FillRect(hdc, &rect, selectBrush);
                DeleteObject(selectBrush);
            }

            SetTextColor(hdc, textColor);
            TextOutA(hdc, x + 15, textY, taskText, strlen(taskText));
            textY += 25;
        }
        SetTextColor(hdc, RGB(0, 0, 0));
    }

    RECT addBtn = { x + 10, y + height - 80, x + 100, y + height - 50 };
    RECT deleteBtn = { x + 110, y + height - 80, x + 200, y + height - 50 };
    RECT completeBtn = { x + 210, y + height - 80, x + 310, y + height - 50 };
    HBRUSH greenBrush = CreateSolidBrush(RGB(76, 175, 80));
    HBRUSH disabledGreenBrush = CreateSolidBrush(RGB(150, 200, 150));
    HBRUSH redBrush = CreateSolidBrush(RGB(244, 67, 54));
    HBRUSH blueBrush = CreateSolidBrush(RGB(33, 150, 243));

    if (isSelectedPast) {
        FillRect(hdc, &addBtn, disabledGreenBrush);
        Rectangle(hdc, addBtn.left, addBtn.top, addBtn.right, addBtn.bottom);
        DrawTextA(hdc, "Add", -1, &addBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    else {
        FillRect(hdc, &addBtn, greenBrush);
        Rectangle(hdc, addBtn.left, addBtn.top, addBtn.right, addBtn.bottom);
        DrawTextA(hdc, "Add", -1, &addBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    FillRect(hdc, &deleteBtn, redBrush);
    Rectangle(hdc, deleteBtn.left, deleteBtn.top, deleteBtn.right, deleteBtn.bottom);
    DrawTextA(hdc, "Delete", -1, &deleteBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    FillRect(hdc, &completeBtn, blueBrush);
    Rectangle(hdc, completeBtn.left, completeBtn.top, completeBtn.right, completeBtn.bottom);
    DrawTextA(hdc, "Complete", -1, &completeBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    DeleteObject(bgBrush); DeleteObject(borderPen); DeleteObject(greenBrush); DeleteObject(disabledGreenBrush); DeleteObject(redBrush); DeleteObject(blueBrush);
}

LRESULT CALLBACK AddTaskDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hCheckTime = NULL;
    static HWND hHourEdit = NULL;
    static HWND hMinuteEdit = NULL;
    static HWND hTimeStatic = NULL;

    switch (msg) {
    case WM_CREATE: {
        CreateWindowA("STATIC", "Title:", WS_CHILD | WS_VISIBLE, 10, 20, 50, 20, hwnd, NULL, NULL, NULL);
        CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, 70, 20, 190, 25, hwnd, (HMENU)101, NULL, NULL);
        CreateWindowA("STATIC", "Description:", WS_CHILD | WS_VISIBLE, 10, 60, 70, 20, hwnd, NULL, NULL, NULL);
        CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE, 70, 60, 190, 60, hwnd, (HMENU)102, NULL, NULL);
        CreateWindowA("STATIC", "Priority:", WS_CHILD | WS_VISIBLE, 10, 135, 60, 20, hwnd, NULL, NULL, NULL);
        HWND hPriority = CreateWindowA("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 70, 135, 100, 100, hwnd, (HMENU)103, NULL, NULL);
        SendMessageA(hPriority, CB_ADDSTRING, 0, (LPARAM)"[LOW] Green");
        SendMessageA(hPriority, CB_ADDSTRING, 0, (LPARAM)"[MEDIUM] Yellow");
        SendMessageA(hPriority, CB_ADDSTRING, 0, (LPARAM)"[HIGH] Red");
        SendMessageA(hPriority, CB_SETCURSEL, 1, 0);
        hCheckTime = CreateWindowA("BUTTON", "Set time", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 10, 170, 80, 20, hwnd, (HMENU)104, NULL, NULL);
        hTimeStatic = CreateWindowA("STATIC", "Time:", WS_CHILD | WS_VISIBLE, 10, 195, 40, 20, hwnd, NULL, NULL, NULL);
        hHourEdit = CreateWindowA("EDIT", "12", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 60, 193, 40, 22, hwnd, (HMENU)105, NULL, NULL);
        CreateWindowA("STATIC", ":", WS_CHILD | WS_VISIBLE, 105, 193, 10, 20, hwnd, NULL, NULL, NULL);
        CreateWindowA("EDIT", "00", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 115, 193, 40, 22, hwnd, (HMENU)106, NULL, NULL);
        ShowWindow(hTimeStatic, SW_HIDE);
        ShowWindow(hHourEdit, SW_HIDE);
        ShowWindow(GetDlgItem(hwnd, 106), SW_HIDE);
        CreateWindowA("BUTTON", "OK", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 50, 230, 80, 30, hwnd, (HMENU)IDOK, NULL, NULL);
        CreateWindowA("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 160, 230, 80, 30, hwnd, (HMENU)IDCANCEL, NULL, NULL);
        if (isPastDate(selectedDay, currentMonth, currentYear)) {
            MessageBoxA(hwnd, "Warning! You are trying to add a task to a past date. This is impossible.", "Warning", MB_OK);
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 104) {
            int isChecked = IsDlgButtonChecked(hwnd, 104);
            ShowWindow(hTimeStatic, isChecked ? SW_SHOW : SW_HIDE);
            ShowWindow(hHourEdit, isChecked ? SW_SHOW : SW_HIDE);
            ShowWindow(GetDlgItem(hwnd, 106), isChecked ? SW_SHOW : SW_HIDE);
            return 0;
        }
        if (LOWORD(wParam) == IDOK) {
            char title[MAX_TITLE] = "", desc[MAX_DESC] = "";
            GetDlgItemTextA(hwnd, 101, title, MAX_TITLE);
            GetDlgItemTextA(hwnd, 102, desc, MAX_DESC);
            int priority = SendDlgItemMessageA(hwnd, 103, CB_GETCURSEL, 0, 0) + 1;
            int hasTime = IsDlgButtonChecked(hwnd, 104);
            int hour = 12, minute = 0;
            if (hasTime) {
                char hourStr[10] = "", minuteStr[10] = "";
                GetDlgItemTextA(hwnd, 105, hourStr, 10);
                GetDlgItemTextA(hwnd, 106, minuteStr, 10);
                hour = atoi(hourStr);
                minute = atoi(minuteStr);
                if (hour < 0) hour = 0;
                if (hour > 23) hour = 23;
                if (minute < 0) minute = 0;
                if (minute > 59) minute = 59;
            }
            if (strlen(title) > 0) {
                addTask(selectedDay, currentMonth, currentYear, title, desc, priority, hasTime, hour, minute);
                DestroyWindow(hwnd);
                InvalidateRect(g_mainHwnd, NULL, TRUE);
            }
            else {
                MessageBoxA(hwnd, "Enter task title!", "Error", MB_OK);
            }
            return 0;
        }
        else if (LOWORD(wParam) == IDCANCEL) { DestroyWindow(hwnd); return 0; }
        break;
    }
    case WM_CLOSE: DestroyWindow(hwnd); return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static int minuteCounter = 0;
    static DWORD lastClickTime = 0;
    static int lastClickedIndex = -1;

    switch (msg) {
    case WM_CREATE: {
        g_mainHwnd = hwnd;
        loadTasksFromFile();
        removeOverdueTasks();
        Date today = getCurrentDate();
        currentMonth = today.month;
        currentYear = today.year;
        selectedDay = today.day;
        g_hPrevBtn = CreateWindowA("BUTTON", "<", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 30, 12, 45, 35, hwnd, (HMENU)1001, GetModuleHandle(NULL), NULL);
        char buf[100];
        sprintf(buf, "%s %d", monthNames[currentMonth - 1], currentYear);
        g_hMonthText = CreateWindowA("STATIC", buf, WS_CHILD | WS_VISIBLE | SS_CENTER, 85, 18, 200, 28, hwnd, NULL, NULL, NULL);
        g_hNextBtn = CreateWindowA("BUTTON", ">", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 295, 12, 45, 35, hwnd, (HMENU)1002, GetModuleHandle(NULL), NULL);
        g_hClockText = CreateWindowA("STATIC", "", WS_CHILD | WS_VISIBLE | SS_RIGHT, 850, 15, 120, 25, hwnd, NULL, NULL, NULL);
        HFONT hClockFont = CreateFontA(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Segoe UI");
        SendMessageA(g_hClockText, WM_SETFONT, (WPARAM)hClockFont, TRUE);
        SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL, NULL);
        SetTimer(hwnd, TIMER_ID_REMINDER, REMINDER_DELAY, NULL);
        updateClock();
        loadBackgroundImage();
        checkOverdueReminders();
        playStartSound();
        return 0;
    }
    case WM_TIMER:
        if (wParam == TIMER_ID) { updateClock(); minuteCounter++; if (minuteCounter >= 60) { minuteCounter = 0; removeOverdueTasks(); } }
        else if (wParam == TIMER_ID_REMINDER) { KillTimer(hwnd, TIMER_ID_REMINDER); checkReminders(); checkOverdueReminders(); removeOverdueTasks(); }
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == 1001) goPreviousMonth();
        else if (LOWORD(wParam) == 1002) goNextMonth();
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps); RECT rect; GetClientRect(hwnd, &rect);
        drawBackground(hdc, rect.right, rect.bottom);
        drawCalendar(hdc, 20, 60, rect.right - 400 - 30, rect.bottom - 80);
        drawTaskPanel(hdc, rect.right - 380, 60, 360, rect.bottom - 80);
        EndPaint(hwnd, &ps);
        break;
    }
    case WM_LBUTTONDOWN: {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        RECT rect; GetClientRect(hwnd, &rect);
        int calendarX = 20, calendarY = 60, calendarW = rect.right - 400 - 30, calendarH = rect.bottom - 80;
        if (pt.x >= calendarX && pt.x <= calendarX + calendarW && pt.y >= calendarY && pt.y <= calendarY + calendarH) {
            int cellW = calendarW / DAYS_IN_WEEK, cellH = (calendarH - 30) / WEEKS_TO_SHOW;
            int firstDayOfWeek = getDayOfWeek(1, currentMonth, currentYear);
            int col = (pt.x - calendarX) / cellW, row = (pt.y - calendarY - 30) / cellH;
            int day = row * DAYS_IN_WEEK + col - firstDayOfWeek + 1;
            if (day >= 1 && day <= getDaysInMonth(currentMonth, currentYear)) {
                selectedDay = day; selectedTaskIndex = -1;
                InvalidateRect(hwnd, NULL, TRUE);
                playDaySelectSound();
            }
        }
        int taskPanelX = rect.right - 380, taskPanelY = 60, taskPanelW = 360, taskPanelH = rect.bottom - 80;
        if (pt.x >= taskPanelX && pt.x <= taskPanelX + taskPanelW && pt.y >= taskPanelY && pt.y <= taskPanelY + taskPanelH) {
            int dayIndex = getDayOfYear(selectedDay, currentMonth, currentYear);
            int taskY = taskPanelY + 40;
            for (int i = 0; i < taskCount[dayIndex]; i++) {
                if (pt.y >= taskY && pt.y <= taskY + 20) {
                    DWORD currentTime = GetTickCount();

                    // Двойной клик (менее 300 мс по той же задаче)
                    if (currentTime - lastClickTime < 300 && i == lastClickedIndex) {
                        // Показываем описание
                        showTaskDescription(selectedDay, currentMonth, currentYear, i);
                    }
                    else {
                        // Обычный клик — выделяем задачу
                        selectedTaskIndex = i;
                        InvalidateRect(hwnd, NULL, TRUE);
                    }
                    lastClickTime = currentTime;
                    lastClickedIndex = i;
                    break;
                }
                taskY += 25;
            }

            RECT addBtn = { taskPanelX + 10, taskPanelY + taskPanelH - 80, taskPanelX + 100, taskPanelY + taskPanelH - 50 };
            if (pt.x >= addBtn.left && pt.x <= addBtn.right && pt.y >= addBtn.top && pt.y <= addBtn.bottom) {
                if (isPastDate(selectedDay, currentMonth, currentYear)) MessageBoxA(hwnd, "Cannot add task to past date!", "Error", MB_OK);
                else { HWND hDlg = CreateWindowExA(0, "AddTaskDlg", "Add Task", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, 400, 300, 320, 300, hwnd, NULL, GetModuleHandle(NULL), NULL); ShowWindow(hDlg, SW_SHOW); }
            }
            RECT deleteBtn = { taskPanelX + 110, taskPanelY + taskPanelH - 80, taskPanelX + 200, taskPanelY + taskPanelH - 50 };
            if (pt.x >= deleteBtn.left && pt.x <= deleteBtn.right && pt.y >= deleteBtn.top && pt.y <= deleteBtn.bottom) {
                if (selectedTaskIndex >= 0) { deleteTask(selectedDay, currentMonth, currentYear, selectedTaskIndex); selectedTaskIndex = -1; InvalidateRect(hwnd, NULL, TRUE); }
                else MessageBoxA(hwnd, "Select a task first!", "Error", MB_OK);
            }
            RECT completeBtn = { taskPanelX + 210, taskPanelY + taskPanelH - 80, taskPanelX + 310, taskPanelY + taskPanelH - 50 };
            if (pt.x >= completeBtn.left && pt.x <= completeBtn.right && pt.y >= completeBtn.top && pt.y <= completeBtn.bottom) {
                if (selectedTaskIndex >= 0) { completeTask(selectedDay, currentMonth, currentYear, selectedTaskIndex); selectedTaskIndex = -1; InvalidateRect(hwnd, NULL, TRUE); }
                else MessageBoxA(hwnd, "Select a task first!", "Error", MB_OK);
            }
        }
        break;
    }
    case WM_DESTROY:
        saveTasksToFile();
        if (g_hBackgroundBitmap) DeleteObject(g_hBackgroundBitmap);
        KillTimer(hwnd, TIMER_ID);
        KillTimer(hwnd, TIMER_ID_REMINDER);
        playExitSound();
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    WNDCLASSA wcDlg = { 0 };
    wcDlg.lpfnWndProc = AddTaskDlgProc;
    wcDlg.hInstance = hInstance;
    wcDlg.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcDlg.lpszClassName = "AddTaskDlg";
    RegisterClassA(&wcDlg);

    WNDCLASSA wcMain = { 0 };
    wcMain.lpfnWndProc = WndProc;
    wcMain.hInstance = hInstance;
    wcMain.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcMain.lpszClassName = "CalendarApp";
    wcMain.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wcMain);

    HWND hwnd = CreateWindowExA(0, "CalendarApp", "Planner - Calendar",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 1000, 700,
        NULL, NULL, hInstance, NULL);
    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}