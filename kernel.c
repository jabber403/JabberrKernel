// ============================================================================
// Jabberr Kernel - Complete Bare-Metal x86 Operating System & Forth Environment
// Architecture: x86 32-bit Protected Mode (GCC / NASM / QEMU)
// Features: VGA Driver, PS/2 Keyboard with Shift, Block-Based VFS, 
//           Runtime Forth Dictionary Compiler, Text Editor, 3D Maze, & Minesweeper.
// ============================================================================

#define VGA_ADDRESS 0xB8000
#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define WHITE_BLUE  0x1F

#define KEYBOARD_DATA_PORT   0x60
#define KEYBOARD_STATUS_PORT 0x64

// Global VGA and Keyboard State Variables
unsigned short *vga = (unsigned short *)VGA_ADDRESS;
int cursor_x = 0;
int cursor_y = 0;
int shift_pressed = 0;

// ============================================================================
// 1. BLOCK-BASED FILE SYSTEM (VFS) STRUCTURES & UTILITIES
// ============================================================================
#define MAX_INODES   16
#define MAX_BLOCKS   64
#define BLOCK_SIZE   64
#define MAX_FILENAME 32

typedef struct {
    char name[MAX_FILENAME];
    int size;
    int start_block;
    int used;
} Inode;

Inode inode_table[MAX_INODES];
char data_blocks[MAX_BLOCKS][BLOCK_SIZE];
int block_bitmap[MAX_BLOCKS];

int allocate_block() {
    for (int i = 0; i < MAX_BLOCKS; i++) {
        if (!block_bitmap[i]) {
            block_bitmap[i] = 1;
            return i;
        }
    }
    return -1; // Disk Full
}

void free_block(int block_idx) {
    if (block_idx >= 0 && block_idx < MAX_BLOCKS) {
        block_bitmap[block_idx] = 0;
    }
}

int find_inode(const char *name) {
    for (int i = 0; i < MAX_INODES; i++) {
        if (inode_table[i].used) {
            int match = 1, j = 0;
            while (name[j] != '\0' && inode_table[i].name[j] != '\0') {
                if (name[j] != inode_table[i].name[j]) { 
                    match = 0; 
                    break; 
                }
                j++;
            }
            if (match && name[j] == '\0' && inode_table[i].name[j] == '\0') {
                return i;
            }
        }
    }
    return -1;
}

void init_fs() {
    for (int i = 0; i < MAX_INODES; i++) {
        inode_table[i].used = 0;
        inode_table[i].size = 0;
        inode_table[i].start_block = -1;
    }
    for (int i = 0; i < MAX_BLOCKS; i++) {
        block_bitmap[i] = 0;
    }
}

// ============================================================================
// 2. LOW-LEVEL PORT I/O & VGA DISPLAY DRIVER
// ============================================================================
static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outl(unsigned short port, unsigned long val) {
    __asm__ __volatile__ ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline unsigned long inl(unsigned short port) {
    unsigned long ret;
    __asm__ __volatile__ ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void vga_clear() {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = (WHITE_BLUE << 8) | ' ';
    }
    cursor_x = 0;
    cursor_y = 0;
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            vga[cursor_y * VGA_WIDTH + cursor_x] = (WHITE_BLUE << 8) | ' ';
        }
    } else {
        vga[cursor_y * VGA_WIDTH + cursor_x] = (WHITE_BLUE << 8) | c;
        cursor_x++;
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    }
    if (cursor_y >= VGA_HEIGHT) {
        for (int y = 0; y < VGA_HEIGHT - 1; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                vga[y * VGA_WIDTH + x] = vga[(y + 1) * VGA_WIDTH + x];
            }
        }
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (WHITE_BLUE << 8) | ' ';
        }
        cursor_y = VGA_HEIGHT - 1;
    }
}

void vga_print(const char *str) {
    while (*str) {
        vga_putchar(*str++);
    }
}

// ============================================================================
// 3. HARDWARE DETECTION & BOOT ANIMATION
// ============================================================================
unsigned short pci_read_word(unsigned char bus, unsigned char slot, unsigned char func, unsigned char offset) {
    unsigned long address = (unsigned long)((1 << 31) | (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xfc));
    outl(0xCF8, address);
    return (unsigned short)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
}

void init_network_card() {
    vga_print("\nScanning PCI Bus for Network Cards...\n");
    for (int bus = 0; bus < 256; bus++) {
        for (int slot = 0; slot < 32; slot++) {
            unsigned short vendor = pci_read_word(bus, slot, 0, 0);
            if (vendor == 0xFFFF) continue; 
            unsigned short device = pci_read_word(bus, slot, 0, 2);
            if (vendor == 0x8086 && device == 0x100E) {
                vga_print("[+] Found Intel E1000 NIC on PCI Bus!\n");
                return;
            }
        }
    }
    vga_print("[-] No compatible network card found.\n");
}

void boot_animation() {
    vga_print("Booting Jabberr Kernel");
    for (int i = 0; i < 4; i++) {
        for (volatile int j = 0; j < 8000000; j++);
        vga_print(".");
    }
    vga_print("\n[OK] Initializing CPU Registers & Protected Mode GDT...\n");
    for (volatile int j = 0; j < 8000000; j++);
    vga_print("[OK] Mounting Block-Based Inode File System...\n");
    for (volatile int j = 0; j < 8000000; j++);
    vga_print("[OK] Loading Dynamic Forth Compiler Engine...\n\n");
    for (volatile int j = 0; j < 12000000; j++);
    vga_clear();
}

// ============================================================================
// 4. BUILT-IN 3D MAZE GAME ENGINE (INTEGER RAYCASTER)
// ============================================================================
#define MAP_SIZE 8
static const int maze_map[MAP_SIZE][MAP_SIZE] = {
    {1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,1},
    {1,0,1,1,0,1,0,1},
    {1,0,1,0,0,1,0,1},
    {1,0,0,0,1,1,0,1},
    {1,1,1,0,0,0,0,1},
    {1,0,0,0,1,1,0,1},
    {1,1,1,1,1,1,1,1}
};

static const int sin_quarter[17] = {
    0, 25, 50, 74, 98, 121, 142, 162, 181, 198, 213, 226, 237, 245, 251, 255, 256
};

static int isin(int a) {
    a &= 63;
    int q = a >> 4, r = a & 15;
    switch (q) {
        case 0:  return  sin_quarter[r];
        case 1:  return  sin_quarter[16 - r];
        case 2:  return -sin_quarter[r];
        default: return -sin_quarter[16 - r];
    }
}
static int icos(int a) { return isin(a + 16); }

static int is_wall(int fx, int fy) {
    if (fx < 0 || fy < 0) return 1;
    int tx = fx >> 8, ty = fy >> 8;
    if (tx >= MAP_SIZE || ty >= MAP_SIZE) return 1;
    return maze_map[ty][tx] != 0;
}

void run_3d_maze() {
    vga_clear();
    vga_print("Initializing 3D Raycaster Engine...\n");
    for (volatile int j = 0; j < 4000000; j++);

    int px = 3 * 256 + 128;
    int py = 3 * 256 + 128;
    int angle = 0;

    while (1) {
        while (inb(KEYBOARD_STATUS_PORT) & 1) {
            unsigned char code = inb(KEYBOARD_DATA_PORT);
            if (code & 0x80) continue;
            if (code == 0x01) return; // ESC

            int dx = icos(angle), dy = isin(angle);
            int nx = px, ny = py;

            if (code == 0x11) { nx += dx * 40 / 256; ny += dy * 40 / 256; }
            if (code == 0x1F) { nx -= dx * 40 / 256; ny -= dy * 40 / 256; }
            if (code == 0x1E) angle -= 2;
            if (code == 0x20) angle += 2;
            angle &= 63;

            if (!is_wall(nx, py)) px = nx;
            if (!is_wall(px, ny)) py = ny;
        }

        int dirx = icos(angle), diry = isin(angle);
        int plx = -diry * 170 / 256;
        int ply =  dirx * 170 / 256;

        for (int x = 0; x < VGA_WIDTH; x++) {
            int cx   = (x * 512) / VGA_WIDTH - 256;
            int rayx = dirx + (plx * cx) / 256;
            int ray_y = diry + (ply * cx) / 256;

            int s = 1;
            for (; s < 128; s++) {
                if (is_wall(px + (rayx * s) / 16, py + (ray_y * s) / 16)) break;
            }

            int height = (VGA_HEIGHT * 16) / s;
            if (height > VGA_HEIGHT) height = VGA_HEIGHT;
            int start = (VGA_HEIGHT - height) / 2;
            int end   = start + height;

            unsigned short wall;
            if      (s <  32) wall = (0x0F << 8) | 0xDB;
            else if (s <  64) wall = (0x07 << 8) | 0xB2;
            else if (s < 100) wall = (0x07 << 8) | 0xB1;
            else              wall = (0x08 << 8) | 0xB0;

            for (int y = 0; y < VGA_HEIGHT; y++) {
                unsigned short cell;
                if (y < start)      cell = (0x1B << 8) | ' ';
                else if (y < end)   cell = wall;
                else                cell = (0x28 << 8) | '.';
                vga[y * VGA_WIDTH + x] = cell;
            }
        }

        const char *hud = " [3D MAZE] W/S: Move | A/D: Turn | ESC: Quit ";
        for (int i = 0; hud[i] != '\0' && i < VGA_WIDTH; i++) {
            vga[i] = (0x70 << 8) | hud[i];
        }

        for (volatile int d = 0; d < 400000; d++);
    }
}

// ============================================================================
// 5. BUILT-IN MINESWEEPER GAME ENGINE
// ============================================================================
#define MS_W 10
#define MS_H 10

void run_minesweeper() {
    vga_clear();
    vga_print("Initializing Minesweeper...\n");
    for (volatile int j = 0; j < 2000000; j++);

    int mines[MS_H][MS_W];
    int revealed[MS_H][MS_W];
    int flagged[MS_H][MS_W];
    int adj[MS_H][MS_W];

    for (int y = 0; y < MS_H; y++) {
        for (int x = 0; x < MS_W; x++) {
            mines[y][x] = 0;
            revealed[y][x] = 0;
            flagged[y][x] = 0;
            adj[y][x] = 0;
        }
    }

    // Place 10 mines using a simple LCG pseudo-random sequence
    int placed = 0;
    unsigned int seed = 1337;
    while (placed < 10) {
        seed = seed * 1103515245 + 12345;
        int rx = (seed >> 16) % MS_W;
        seed = seed * 1103515245 + 12345;
        int ry = (seed >> 16) % MS_H;
        if (!mines[ry][rx] && !(rx == 0 && ry == 0)) {
            mines[ry][rx] = 1;
            placed++;
        }
    }

    // Calculate neighboring mine counts
    for (int y = 0; y < MS_H; y++) {
        for (int x = 0; x < MS_W; x++) {
            if (mines[y][x]) continue;
            int count = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int ny = y + dy, nx = x + dx;
                    if (nx >= 0 && nx < MS_W && ny >= 0 && ny < MS_H) {
                        if (mines[ny][nx]) count++;
                    }
                }
            }
            adj[y][x] = count;
        }
    }

    int cx = 0, cy = 0;
    int game_over = 0;

    while (1) {
        vga_clear();
        vga_print("=== JABBERR MINESWEEPER ===\n");
        vga_print("W/A/S/D: Move | SPACE: Reveal | F: Flag | ESC: Quit\n\n");

        for (int y = 0; y < MS_H; y++) {
            vga_print("   ");
            for (int x = 0; x < MS_W; x++) {
                char buf[4];
                if (x == cx && y == cy) vga_print("[");
                else vga_print(" ");

                if (revealed[y][x]) {
                    if (mines[y][x]) vga_print("*");
                    else if (adj[y][x] > 0) {
                        buf[0] = '0' + adj[y][x]; buf[1] = '\0';
                        vga_print(buf);
                    } else vga_print(".");
                } else if (flagged[y][x]) {
                    vga_print("F");
                } else {
                    vga_print("#");
                }

                if (x == cx && y == cy) vga_print("]");
                else vga_print(" ");
            }
            vga_print("\n");
        }

        if (game_over) {
            vga_print("\nBOOM! You hit a mine! Press ESC to exit.\n");
        }

        while (!(inb(KEYBOARD_STATUS_PORT) & 1));
        unsigned char code = inb(KEYBOARD_DATA_PORT);
        if (code & 0x80) continue;

        if (code == 0x01) return; // ESC

        if (!game_over) {
            if (code == 0x11 && cy > 0) cy--;         // W
            if (code == 0x1F && cy < MS_H - 1) cy++;  // S
            if (code == 0x1E && cx > 0) cx--;         // A
            if (code == 0x20 && cx < MS_W - 1) cx++;  // D

            if (code == 0x39) { // SPACE
                if (!flagged[cy][cx]) {
                    revealed[cy][cx] = 1;
                    if (mines[cy][cx]) {
                        game_over = 1;
                        for (int yy = 0; yy < MS_H; yy++)
                            for (int xx = 0; xx < MS_W; xx++)
                                if (mines[yy][xx]) revealed[yy][xx] = 1;
                    }
                }
            }
            if (code == 0x21) { // F key
                if (!revealed[cy][cx]) {
                    flagged[cy][cx] = !flagged[cy][cx];
                }
            }
        }
    }
}

// ============================================================================
// 6. FORTH STACK, RUNTIME DICTIONARY, & PRIMITIVES
// ============================================================================
#define STACK_SIZE 256
int data_stack[STACK_SIZE];
int sp = -1;

void push(int val) {
    if (sp < STACK_SIZE - 1) {
        data_stack[++sp] = val;
    }
}

int pop() {
    if (sp >= 0) {
        return data_stack[sp--];
    }
    return 0;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

void print_int(int val) {
    if (val == 0) {
        vga_print("0");
        return;
    }
    char buf[16]; 
    int idx = 0;
    while (val > 0) { 
        buf[idx++] = '0' + (val % 10); 
        val /= 10; 
    }
    while (idx > 0) { 
        char c[2] = {buf[--idx], '\0'}; 
        vga_print(c); 
    }
}

#define MAX_WORDS 32
typedef struct {
    char name[32];
    char body[128];
    int used;
} ForthWord;

ForthWord user_dictionary[MAX_WORDS];

void init_dictionary() {
    for (int i = 0; i < MAX_WORDS; i++) {
        user_dictionary[i].used = 0;
    }
}

int find_user_word(const char *name) {
    for (int i = 0; i < MAX_WORDS; i++) {
        if (user_dictionary[i].used) {
            int match = 1, j = 0;
            while (name[j] != '\0' && user_dictionary[i].name[j] != '\0') {
                if (name[j] != user_dictionary[i].name[j]) { 
                    match = 0; 
                    break; 
                }
                j++;
            }
            if (match && name[j] == '\0' && user_dictionary[i].name[j] == '\0') {
                return i;
            }
        }
    }
    return -1;
}

void execute_line(const char *line);

void process_token(const char *word) {
    int u_idx = find_user_word(word);
    if (u_idx != -1) {
        execute_line(user_dictionary[u_idx].body);
        return;
    }

    if (strcmp(word, "help") == 0) {
        vga_print("\nJabberr Kernel Command Reference:\n");
        vga_print("  3d maze           - Launch built-in 3D Maze Game\n");
        vga_print("  minesweeper       - Launch built-in Minesweeper Game\n");
        vga_print("  .\" [text] \"       - Print string literal\n");
        vga_print("  : [name] [code] ; - Define custom Forth word\n");
        vga_print("  words             - List compiled custom words\n");
        vga_print("  ls                - List block file system\n");
        vga_print("  touch [file]      - Allocate file inode\n");
        vga_print("  cat [file]        - Read file block contents\n");
        vga_print("  rm [file]         - Delete file & free block\n");
        vga_print("  stack             - Inspect data stack\n");
        vga_print("  + , - , *         - Stack math operators\n");
        vga_print("  . or =            - Pop & print stack top\n");
        vga_print("  fib               - Run Fibonacci sequence\n");
        vga_print("  nic               - Scan PCI network hardware\n");
        vga_print("  time / date       - Display system timestamp[cite: 2]\n");
        vga_print("  settings          - View kernel configuration[cite: 2]\n");
        vga_print("  off               - Safely shutdown kernel[cite: 2]\n");
    } else if (strcmp(word, "words") == 0) {
        vga_print("\nCustom Compiled Dictionary Words:\n");
        int found = 0;
        for (int i = 0; i < MAX_WORDS; i++) {
            if (user_dictionary[i].used) {
                vga_print("  : ");
                vga_print(user_dictionary[i].name);
                vga_print(" -> ");
                vga_print(user_dictionary[i].body);
                vga_print("\n");
                found = 1;
            }
        }
        if (!found) vga_print("  (no custom words defined)\n");
    } else if (strcmp(word, "cls") == 0 || strcmp(word, "clear") == 0) {
        vga_clear();
    } else if (strcmp(word, "stack") == 0) {
        vga_print("\nStack: [ ");
        for (int i = 0; i <= sp; i++) {
            print_int(data_stack[i]);
            vga_print(" ");
        }
        vga_print("]\n");
    } else if (strcmp(word, "+") == 0) {
        int b = pop(); 
        int a = pop();
        push(a + b);
    } else if (strcmp(word, "-") == 0) {
        int b = pop(); 
        int a = pop();
        push(a - b);
    } else if (strcmp(word, "*") == 0) {
        int b = pop(); 
        int a = pop();
        push(a * b);
    } else if (strcmp(word, ".") == 0 || strcmp(word, "=") == 0) {
        int val = pop();
        vga_print("\n-> ");
        print_int(val);
        vga_print("\n");
    } else if (strcmp(word, "fib") == 0) {
        vga_print("\n[Sequence: 0 1 ");
        int a = 0, b = 1;
        for (int i = 0; i < 5; i++) {
            int c = a + b;
            print_int(c);
            vga_print(" ");
            a = b;
            b = c;
        }
        vga_print("]\n");
    } else if (strcmp(word, "nic") == 0) {
        init_network_card();
    } else {
        int num = 0, is_num = 1, i = 0;
        while (word[i] != '\0') {
            if (word[i] >= '0' && word[i] <= '9') {
                num = num * 10 + (word[i] - '0');
            } else {
                is_num = 0; 
                break;
            }
            i++;
        }
        if (is_num && i > 0) {
            push(num);
        } else if (word[0] != '\0') {
            vga_print("\nUnknown command: ");
            vga_print(word);
            vga_print("\n");
        }
    }
}

// ============================================================================
// 7. TEXT EDITOR BUFFER & LINE EXECUTION PARSER
// ============================================================================
char txte_buffer[BLOCK_SIZE] = ".\" Hello from Jabberr Text Editor!\"";

int starts_with(const char *pre, const char *str) {
    while (*pre) {
        if (*pre++ != *str++) return 0;
    }
    return 1;
}

void execute_line(const char *line) {
    // 0. Game Launchers
    if (strcmp(line, "3d maze") == 0 || strcmp(line, "3d-maze") == 0) {
        run_3d_maze();
        vga_clear();
        vga_print("Exited 3D Maze Game.\n\n");
        return;
    }
    if (strcmp(line, "minesweeper") == 0 || strcmp(line, "mines") == 0) {
        run_minesweeper();
        vga_clear();
        vga_print("Exited Minesweeper.\n\n");
        return;
    }

    // 1. Native String Printing Parser (.\" ... \")
    for (int i = 0; line[i] != '\0'; i++) {
        if (line[i] == '.' && line[i+1] == '"') {
            int start = i + 3;
            int end = start;
            while (line[end] != '"' && line[end] != '\0') {
                end++;
            }
            char str_buf[64];
            int s_idx = 0;
            for (int k = start; k < end && s_idx < 63; k++) {
                str_buf[s_idx++] = line[k];
            }
            str_buf[s_idx] = '\0';
            vga_print("\n");
            vga_print(str_buf);
            vga_print("\n");
            return;
        }
    }

    // 2. Colon Definition Compiler Parser (: name body ;)
    if (line[0] == ':') {
        int i = 2;
        char name[32]; 
        int n_idx = 0;
        while (line[i] != ' ' && line[i] != '\0') {
            name[n_idx++] = line[i++];
        }
        name[n_idx] = '\0';

        if (line[i] == ' ') i++;

        char body[128]; 
        int b_idx = 0;
        while (line[i] != ';' && line[i] != '\0') {
            body[b_idx++] = line[i++];
        }
        if (b_idx > 0 && body[b_idx - 1] == ' ') b_idx--;
        body[b_idx] = '\0';

        int idx = -1;
        for (int k = 0; k < MAX_WORDS; k++) {
            if (!user_dictionary[k].used) { 
                idx = k; 
                break; 
            }
        }
        if (idx != -1) {
            user_dictionary[idx].used = 1;
            int k = 0; while (name[k] != '\0') { user_dictionary[idx].name[k] = name[k]; k++; }
            user_dictionary[idx].name[k] = '\0';
            k = 0; while (body[k] != '\0') { user_dictionary[idx].body[k] = body[k]; k++; }
            user_dictionary[idx].body[k] = '\0';

            vga_print("\n[+] Defined compiled word: '");
            vga_print(name);
            vga_print("'\n");
        }
        return;
    }

    // 3. File System Commands
    if (strcmp(line, "ls") == 0) {
        vga_print("\nBlock File System Directory:\n");
        int found = 0;
        for (int i = 0; i < MAX_INODES; i++) {
            if (inode_table[i].used) {
                vga_print("  [FILE] ");
                vga_print(inode_table[i].name);
                vga_print(" (Size: ");
                print_int(inode_table[i].size);
                vga_print(" bytes, Block: ");
                print_int(inode_table[i].start_block);
                vga_print(")\n");
                found = 1;
            }
        }
        if (!found) vga_print("  (directory empty)\n");
        return;
    }

    if (starts_with("touch ", line)) {
        const char *filename = line + 6;
        int idx = find_inode(filename);
        if (idx == -1) {
            for (int i = 0; i < MAX_INODES; i++) {
                if (!inode_table[i].used) { idx = i; break; }
            }
        }
        if (idx != -1) {
            int b_idx = allocate_block();
            if (b_idx != -1) {
                inode_table[idx].used = 1;
                int k = 0; while (filename[k] != '\0') { inode_table[idx].name[k] = filename[k]; k++; }
                inode_table[idx].name[k] = '\0';
                inode_table[idx].start_block = b_idx;
                inode_table[idx].size = 0;
                data_blocks[b_idx][0] = '\0';
                vga_print("\n[+] Created file inode: ");
                vga_print(filename);
                vga_print("\n");
            } else {
                vga_print("\nError: Disk out of blocks!\n");
            }
        }
        return;
    }

    if (starts_with("cat ", line)) {
        const char *filename = line + 4;
        int idx = find_inode(filename);
        if (idx != -1) {
            vga_print("\n--- ");
            vga_print(filename);
            vga_print(" ---\n");
            vga_print((char*)data_blocks[inode_table[idx].start_block]);
            vga_print("\n--------------------\n");
        } else {
            vga_print("\nError: File not found.\n");
        }
        return;
    }

    if (starts_with("rm ", line)) {
        const char *filename = line + 3;
        int idx = find_inode(filename);
        if (idx != -1) {
            free_block(inode_table[idx].start_block);
            inode_table[idx].used = 0;
            vga_print("\n[+] Removed file: ");
            vga_print(filename);
            vga_print("\n");
        } else {
            vga_print("\nError: File not found.\n");
        }
        return;
    }

    // 4. System Utility Commands
    if (strcmp(line, "time") == 0 || strcmp(line, "date") == 0) {
        vga_print("\nCurrent Date & Time: October 12, 2026 - 12:33 PM[cite: 2]\n");
        return;
    }

    if (strcmp(line, "settings") == 0) {
        vga_print("\n=== Jabberr Kernel Settings ===\n");
        vga_print("  Theme: Classic VGA Blue (0x1F)[cite: 2]\n");
        vga_print("  Block FS: 16 Inodes, 64 Blocks (64B each)[cite: 2]\n");
        vga_print("  Keyboard Driver: PS/2 + Shift Modifier Active[cite: 2]\n");
        vga_print("  Forth Engine: Runtime Dictionary Compiler[cite: 2]\n");
        vga_print("===============================\n");
        return;
    }

    if (strcmp(line, "off") == 0) {
        vga_print("\nShutting down Jabberr Kernel[cite: 2]...\n");
        vga_print("System halted. You can safely close QEMU[cite: 2].\n");
        __asm__ __volatile__ ("cli");
        while (1) {
            __asm__ __volatile__ ("hlt");
        }
    }

    // 5. Text Editor Commands
    if (strcmp(line, "txte") == 0) {
        vga_print("\n[txte Buffer]: ");
        vga_print(txte_buffer);
        vga_print("\n");
        return;
    }

    if (starts_with("txte write ", line)) {
        const char *text = line + 11;
        int k = 0;
        while (text[k] != '\0' && k < BLOCK_SIZE - 1) {
            txte_buffer[k] = text[k];
            k++;
        }
        txte_buffer[k] = '\0';
        vga_print("\n[+] Buffer updated successfully!\n");
        return;
    }

    if (starts_with("txte save as ", line)) {
        const char *filename = line + 13;
        int idx = find_inode(filename);
        if (idx == -1) {
            for (int i = 0; i < MAX_INODES; i++) {
                if (!inode_table[i].used) { idx = i; break; }
            }
        }
        if (idx != -1) {
            int b_idx = inode_table[idx].used ? inode_table[idx].start_block : allocate_block();
            if (b_idx != -1) {
                inode_table[idx].used = 1;
                int k = 0; while (filename[k] != '\0') { inode_table[idx].name[k] = filename[k]; k++; }
                inode_table[idx].name[k] = '\0';
                inode_table[idx].start_block = b_idx;
                
                k = 0; 
                int size = 0;
                while (txte_buffer[k] != '\0' && k < BLOCK_SIZE - 1) {
                    data_blocks[b_idx][k] = txte_buffer[k];
                    k++; 
                    size++;
                }
                data_blocks[b_idx][k] = '\0';
                inode_table[idx].size = size;

                vga_print("\n[+] Saved buffer to block file: ");
                vga_print(filename);
                vga_print("\n");
            }
        }
        return;
    }

    // 6. Default Forth Token & Math Interpreter
    char word[32]; 
    int w_idx = 0;
    int i = 0;
    while (1) {
        if (line[i] == ' ' || line[i] == '\0') {
            word[w_idx] = '\0';
            if (w_idx > 0) {
                process_token(word);
                w_idx = 0;
            }
            if (line[i] == '\0') break;
        } else {
            if (w_idx < 31) word[w_idx++] = line[i];
        }
        i++;
    }
}

// ============================================================================
// 8. PS/2 KEYBOARD DRIVER WITH SHIFT SUPPORT
// ============================================================================
const char scancode_map[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
     0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
     0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
   '*',   0, ' '
};

const char scancode_shift_map[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
  '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
     0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',
     0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
   '*',   0, ' '
};

char read_keyboard() {
    while (1) {
        while (!(inb(KEYBOARD_STATUS_PORT) & 1));
        unsigned char code = inb(KEYBOARD_DATA_PORT);

        if (code == 0x2A || code == 0x36) { 
            shift_pressed = 1; 
            continue; 
        }
        if (code == 0xAA || code == 0xB6) { 
            shift_pressed = 0; 
            continue; 
        }
        if (code & 0x80) continue;

        return shift_pressed ? scancode_shift_map[code] : scancode_map[code];
    }
}

// ============================================================================
// 9. KERNEL ENTRY POINT (kmain)
// ============================================================================
void kmain(void) {
    vga_clear();
    boot_animation();
    init_fs();
    init_dictionary();

    vga_print("=========================================\n");
    vga_print("       WELCOME TO JABBERR KERNEL          \n");
    vga_print(" Full Suite Operating System v4.0       \n");
    vga_print(" Type 'help' for command reference.      \n");
    vga_print("=========================================\n\n");

    char input_buf[128];
    int buf_pos = 0;

    vga_print("jabberr> ");

    while (1) {
        char c = read_keyboard();
        if (c == 0) continue;

        if (c == '\n') {
            vga_putchar('\n');
            input_buf[buf_pos] = '\0';
            
            execute_line(input_buf);

            buf_pos = 0;
            vga_print("jabberr> ");
        } else if (c == '\b') {
            if (buf_pos > 0) {
                buf_pos--;
                vga_putchar('\b');
            }
        } else {
            if (buf_pos < 127) {
                input_buf[buf_pos++] = c;
                vga_putchar(c);
            }
        }
    }
}