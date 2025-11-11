#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <windows.h>
#include <cstdlib>
#include <ctime>
#include <conio.h> 

using namespace std;

static const int W = 10; // 보드 가로
static const int H = 20; // 보드 세로

// 7종 테트리스 조각 모양 (4x4 매트릭스)
// rot=0 기본 모양만 저장
const vector<vector<string>> SHAPES = {
    { "....", "####", "....", "...." }, // I
    { ".##.", ".##.", "....", "...." }, // O
    { "....", ".###", "..#.", "...." }, // T
    { "....", "..##", ".##.", "...." }, // S
    { "....", ".##.", "..##", "...." }, // Z
    { "....", ".###", ".#..", "...." }, // J
    { "....", ".###", "...#", "...." }  // L
};

// 테트리스 조각
class Piece {
public:
    int x, y;   // 좌상단 좌표
    int type;   // 0~6
    int rot;    // 회전 상태 0~3
    int size = 4;

    Piece(int t = 0) : x(W / 2 - 2), y(0), type(t), rot(0) {}
};

// Windows 콘솔 ANSI VT 활성화
bool enableVT() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return false;
    DWORD mode = 0;
    if (!GetConsoleMode(hOut, &mode)) return false;
    if ((mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) == 0) {
        if (!SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) return false;
    }
    return true;
}

// 좌표 회전
char getCell(int type, int rot, int r, int c) {
    int size = 4;
    int r2 = (rot == 0) ? r : (rot == 1) ? c : (rot == 2) ? size - 1 - r : size - 1 - c;
    int c2 = (rot == 0) ? c : (rot == 1) ? size - 1 - r : (rot == 2) ? size - 1 - c : r;
    return SHAPES[type][r2][c2];
}

// 블록 이동, 회전 가능 여부 확인
bool canMove(const vector<string>& board, const Piece& p, int dx, int dy, int nextRot = -1) {
    int rot = (nextRot == -1) ? p.rot : nextRot % 4;

    for (int r = 0; r < p.size; ++r) {
        for (int c = 0; c < p.size; ++c) {
            if (getCell(p.type, rot, r, c) == '#') {
                int nx = p.x + c + dx;
                int ny = p.y + r + dy;

                if (nx < 0 || nx >= W || ny < 0 || ny >= H) return false;
                if (board[ny][nx] != '.') return false;
            }
        }
    }
    return true;
}

// 블록 고정
void lockPiece(vector<string>& board, const Piece& p) {
    for (int r = 0; r < p.size; ++r)
        for (int c = 0; c < p.size; ++c)
            if (getCell(p.type, p.rot, r, c) == '#') {
                int xx = p.x + c;
                int yy = p.y + r;
                if (0 <= xx && xx < W && 0 <= yy && yy < H)
                    board[yy][xx] = '#';
            }
}

// 줄 완성 시 삭제
void clearLines(vector<string>& board) {
    for (int y = H - 1; y >= 0; --y) {
        if (board[y].find('.') == string::npos) { // 빈칸 없으면
            for (int row = y; row > 0; --row)
                board[row] = board[row - 1]; // 위 줄 내리기
            board[0] = string(W, '.'); // 최상단은 빈칸
            ++y; // 같은 줄 다시 검사
        }
    }
}

// 새 블록 생성
Piece spawnRandomPiece() {
    return Piece(rand() % 7);
}

// 프레임 합성
vector<string> composeFrame(const vector<string>& board, const Piece& p) {
    vector<string> buf = board;
    for (int r = 0; r < p.size; ++r)
        for (int c = 0; c < p.size; ++c)
            if (getCell(p.type, p.rot, r, c) == '#') {
                int xx = p.x + c;
                int yy = p.y + r;
                if (0 <= xx && xx < W && 0 <= yy && yy < H)
                    buf[yy][xx] = '#';
            }
    return buf;
}

// 프레임 출력
void drawFrame(const vector<string>& buf) {
    cout << "\x1b[H";
    cout << "+" << string(W, '-') << "+\n";
    for (int r = 0; r < H; ++r)
        cout << "|" << buf[r] << "|\n";
    cout << "+" << string(W, '-') << "+\n";
    cout.flush();
}

int main() {
    srand((unsigned)time(nullptr));
    enableVT();
    cout << "\x1b[2J\x1b[H"; // Clear
    cout << "\x1b[?25l";     // Hide cursor

    vector<string> board(H, string(W, '.'));
    Piece piece = spawnRandomPiece();

    const int fallMs = 400;
    auto lastFall = chrono::steady_clock::now();

    while (true) {
        auto now = chrono::steady_clock::now();

        // 입력 처리
        /* 회전: z,x
           방향키 왼쪽: 왼쪽 이동
           방향기 오른쪽: 오른쪽 이동
           방향기 아래: 빠르게 drop
           */

        if (_kbhit()) {
            char ch = _getch();
            if (ch == 0 || ch == -32) ch = _getch(); // 화살표 처리

            if (ch == 75) { // 왼쪽 이동
                if (canMove(board, piece, -1, 0)) piece.x--;
            }
            else if (ch == 77) { // 오른쪽 이동
                if (canMove(board, piece, 1, 0)) piece.x++;
            }
            else if (ch == 80) { // 빠르게 drop
                if (canMove(board, piece, 0, 1)) piece.y++;
            }
            else if (ch == 'z' || ch == 'Z') { // 반시계 회전
                int nextRot = (piece.rot + 3) % 4;
                if (canMove(board, piece, 0, 0, nextRot)) piece.rot = nextRot;
            }
            else if (ch == 'x' || ch == 'X') { // 시계 회전
                int nextRot = (piece.rot + 1) % 4;
                if (canMove(board, piece, 0, 0, nextRot)) piece.rot = nextRot;
            }
        }

        // drop
        if (chrono::duration_cast<chrono::milliseconds>(now - lastFall).count() >= fallMs) {
            if (canMove(board, piece, 0, 1)) {
                piece.y++;
            }
            else {
                lockPiece(board, piece);
                clearLines(board);
                piece = spawnRandomPiece();
            }
            lastFall = now;
        }

        // 렌더링
        drawFrame(composeFrame(board, piece));
        this_thread::sleep_for(chrono::milliseconds(16));
    }

    cout << "\x1b[?25h";
    return 0;
}
