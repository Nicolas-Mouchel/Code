#include <windows.h>
#include <shellapi.h>
#include <time.h>
#include <stdio.h>

// Définition des constantes pour les temps
const DWORD INACTIVITY_THRESHOLD = 1000 *60*  2; // Temps d'inactivité en ms (2 minutes)
const DWORD SIMULATION_INTERVAL = 1000 *60*  2;  // Intervalle entre les simulations en ms (2 minutes)
DWORD CYCLE_TIME = 1000;                           // Temps de cycle en ms (modifiable)

// Définition des constantes pour la barre de notification
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001

// Variables globales
NOTIFYICONDATA nid;
HWND hwnd;
BOOL isSimulatingAction = FALSE; // Indique si une action simulée est en cours

// Déclarations explicites des fonctions
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void AddTrayIcon(HINSTANCE hInstance);
void RemoveTrayIcon();
DWORD GetIdleTime();
HWND FindTeamsWindow();
void SimulateMouseClickOnWindow(HWND hwnd);
void SimulateMouseMoveOnWindow(HWND hwnd);
void ForceWindowToForeground(HWND hwnd);
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);

// Fonction pour ajouter une icône dans la barre de notification
void AddTrayIcon(HINSTANCE hInstance) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1)); // ID de l'icône = 1
    lstrcpy(nid.szTip, TEXT("NoSleep actif"));
    Shell_NotifyIcon(NIM_ADD, &nid);
}

// Fonction pour supprimer l'icône de la barre de notification
void RemoveTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

// Fonction pour détecter le temps d'inactivité
DWORD GetIdleTime() {
    LASTINPUTINFO lii = {0};
    lii.cbSize = sizeof(LASTINPUTINFO);
    if (GetLastInputInfo(&lii)) {
        return GetTickCount() - lii.dwTime; // Temps écoulé depuis la dernière interaction
    }
    return 0; // En cas d'échec
}

// Fonction pour trouver la fenêtre Teams
HWND FindTeamsWindow() {
    HWND teamsWindow = NULL;
    EnumWindows(EnumWindowsProc, (LPARAM)&teamsWindow);
    return teamsWindow;
}

// Fonction pour simuler un clic de souris sur une fenêtre
void SimulateMouseClickOnWindow(HWND hwnd) {
    isSimulatingAction = TRUE; // Marque le début de l'action simulée

    RECT rect;
    GetWindowRect(hwnd, &rect);

    int x = rect.left + (rect.right - rect.left) / 2;
    int y = rect.top + (rect.bottom - rect.top) / 2;

    INPUT inputs[3] = {0};

    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dx = (x * 65536) / GetSystemMetrics(SM_CXSCREEN);
    inputs[0].mi.dy = (y * 65536) / GetSystemMetrics(SM_CYSCREEN);
    inputs[0].mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;

    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    inputs[2].type = INPUT_MOUSE;
    inputs[2].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    SendInput(3, inputs, sizeof(INPUT));

    isSimulatingAction = FALSE; // Marque la fin de l'action simulée
}

// Fonction pour simuler un mouvement de souris sur une fenêtre
void SimulateMouseMoveOnWindow(HWND hwnd) {
    isSimulatingAction = TRUE; // Marque le début de l'action simulée

    RECT rect;
    GetWindowRect(hwnd, &rect);

    // Calculer les coordonnées du centre de la fenêtre
    int x = rect.left + (rect.right - rect.left) / 2;
    int y = rect.top + (rect.bottom - rect.top) / 2;

    INPUT inputs[1] = {0};

    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dx = (x * 65536) / GetSystemMetrics(SM_CXSCREEN); // Conversion en coordonnées absolues
    inputs[0].mi.dy = (y * 65536) / GetSystemMetrics(SM_CYSCREEN); // Conversion en coordonnées absolues
    inputs[0].mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;

    SendInput(1, inputs, sizeof(INPUT));

    isSimulatingAction = FALSE; // Marque la fin de l'action simulée
}

// Fonction pour mettre une fenêtre au premier plan
void ForceWindowToForeground(HWND hwnd) {
    DWORD foregroundThreadId = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    DWORD targetThreadId = GetWindowThreadProcessId(hwnd, NULL);

    // Attache le thread de la fenêtre cible au thread de la fenêtre au premier plan
    AttachThreadInput(foregroundThreadId, targetThreadId, TRUE);

    // Vérifie si la fenêtre est minimisée ou masquée
    WINDOWPLACEMENT placement;
    GetWindowPlacement(hwnd, &placement);
    if (placement.showCmd == SW_SHOWMINIMIZED) {
        printf("La fenêtre Teams est minimisée. Tentative de restauration...\n");
        ShowWindow(hwnd, SW_RESTORE);
    } else if (placement.showCmd == SW_HIDE) {
        printf("La fenêtre Teams est masquée. Tentative d'affichage...\n");
        ShowWindow(hwnd, SW_SHOW);
    }

    // Autorise explicitement la modification du focus
    AllowSetForegroundWindow(ASFW_ANY);
    SetForegroundWindow(hwnd);

    // Simule une interaction utilisateur (clic ou appui sur ALT)
    keybd_event(VK_MENU, 0, 0, 0); // ALT down
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0); // ALT up

    // Tente de mettre la fenêtre au-dessus
    SwitchToThisWindow(hwnd, TRUE);

    // Détache les threads
    AttachThreadInput(foregroundThreadId, targetThreadId, FALSE);
}

// Fonction de rappel pour énumérer les fenêtres
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    char windowTitle[256];
    GetWindowText(hwnd, windowTitle, sizeof(windowTitle));

    // Vérifie si le titre se termine par " | Microsoft Teams"
    const char* targetSuffix = " | Microsoft Teams";
    size_t titleLen = strlen(windowTitle);
    size_t suffixLen = strlen(targetSuffix);

    if (titleLen >= suffixLen && strcmp(windowTitle + (titleLen - suffixLen), targetSuffix) == 0) {
        *((HWND*)lParam) = hwnd; // Stocke le handle de la fenêtre trouvée
        return FALSE; // Arrête l'énumération
    }

    return TRUE; // Continue l'énumération
}

// Fonction principale
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "NoSleepClass";

    // Afficher l'heure de lancement
    time_t now = time(NULL);
    struct tm* localTime = localtime(&now);
    printf("NoSleep lancé à : %02d:%02d:%02d\n", localTime->tm_hour, localTime->tm_min, localTime->tm_sec);

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    hwnd = CreateWindowEx(0, CLASS_NAME, "NoSleep", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    AddTrayIcon(hInstance);

    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);

    // Hotkey globale : Ctrl + Shift + Q
    RegisterHotKey(hwnd, 1, MOD_CONTROL | MOD_SHIFT, 0x51); // 0x51 = Q

    DWORD lastSimTime = GetTickCount();
    DWORD lastCheckTime = GetTickCount(); // Ajout pour vérifier périodiquement
    BOOL wasInactive = FALSE; // Variable pour suivre l'état précédent de l'inactivité
    BOOL isSimulating = FALSE; // Variable pour suivre l'état de la simulation

    MSG msg;
    while (1) {
        // Vérifie si un message est disponible
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            // Quitter si le message est WM_QUIT
            if (msg.message == WM_QUIT) {
                break;
            }
        }

        // Vérification périodique selon le temps de cycle
        if (GetTickCount() - lastCheckTime > CYCLE_TIME) {
            lastCheckTime = GetTickCount();

            // Vérifie l'inactivité utilisateur
            DWORD idleTime = GetIdleTime();

            if (!isSimulatingAction && idleTime > INACTIVITY_THRESHOLD) {
                if (!wasInactive) { // Affiche le message uniquement si l'inactivité vient d'être détectée
                    printf("Inactivité détectée, démarrage de la simulation...\n");
                    wasInactive = TRUE; // Met à jour l'état
                }
                isSimulating = TRUE;
            } else if (idleTime < CYCLE_TIME / 2) { // Vérifie si le temps d'inactivité est inférieur à la moitié du temps de cycle
                if (wasInactive) { // Affiche le message uniquement si l'activité vient de reprendre
                    printf("Activité détectée, arrêt de la simulation...\n");
                    wasInactive = FALSE; // Met à jour l'état
                }
                isSimulating = FALSE;
            }

            // Ajout de la simulation d'activité
            if (isSimulating) {
                // Simulation d'activité
                if (GetTickCount() - lastSimTime > SIMULATION_INTERVAL) {
                    HWND teamsWindow = FindTeamsWindow();
                    if (teamsWindow) {
                        HWND activeWindow = GetForegroundWindow();
                        if (activeWindow == teamsWindow) {
                            printf("La fenêtre Teams est déjà active.\n");
                            SimulateMouseClickOnWindow(teamsWindow); // Simule un clic sur la fenêtre Teams
                        } else {
                            printf("La fenêtre Teams n'est pas active, tentative de mise au premier plan...\n");
                            ForceWindowToForeground(teamsWindow);

                            // Vérifiez si la fenêtre Teams est maintenant active
                            activeWindow = GetForegroundWindow();
                            if (activeWindow == teamsWindow) {
                                printf("Fenêtre Teams mise au premier plan avec succès.\n");
                                SimulateMouseClickOnWindow(teamsWindow); // Simule un clic sur la fenêtre Teams
                            } else {
                                printf("Échec de mise au premier plan de la fenêtre Teams.\n");
                            }
                        }
                        lastSimTime = GetTickCount(); // Enregistre le moment de la simulation
                    } else {
                        printf("Fenêtre Teams non trouvée.\n");
                    }
                }
            }
        }
    }

    RemoveTrayIcon();

    return 0;
}

// Fonction de traitement des messages de la fenêtre
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                // Afficher le menu contextuel sur clic droit
                POINT cursorPos;
                GetCursorPos(&cursorPos);

                HMENU hMenu = CreatePopupMenu();
                InsertMenu(hMenu, 0, MF_BYPOSITION | MF_STRING, ID_TRAY_EXIT, "Quitter");

                SetForegroundWindow(hwnd); // Assurer que la fenêtre est au premier plan
                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, cursorPos.x, cursorPos.y, 0, hwnd, NULL);
            }
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_TRAY_EXIT:
                    // Quitter l'application
                    PostQuitMessage(0);
                    break;
            }
            break;
        case WM_HOTKEY:
            if (wParam == 1) {
                // Ctrl + Shift + Q a été pressé
                printf("Raccourci clavier détecté : Ctrl + Shift + Q\n");

                HWND teamsWindow = FindTeamsWindow();
                if (teamsWindow) {
                    HWND activeWindow = GetForegroundWindow();
                    if (activeWindow == teamsWindow) {
                        printf("La fenêtre Teams est déjà active.\n");
                        SimulateMouseClickOnWindow(teamsWindow); // Simule un clic sur la fenêtre Teams
                    } else {
                        printf("La fenêtre Teams n'est pas active, tentative de mise au premier plan...\n");
                        ForceWindowToForeground(teamsWindow);

                        // Vérifiez si la fenêtre Teams est maintenant active
                        activeWindow = GetForegroundWindow();
                        if (activeWindow == teamsWindow) {
                            printf("Fenêtre Teams mise au premier plan avec succès.\n");
                            SimulateMouseClickOnWindow(teamsWindow); // Simule un clic sur la fenêtre Teams
                        } else {
                            printf("Échec de mise au premier plan de la fenêtre Teams.\n");
                        }
                    }
                } else {
                    printf("Fenêtre Teams non trouvée.\n");
                }
            }
            break;
        case WM_DESTROY:
            RemoveTrayIcon();
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}
