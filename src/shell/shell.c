// shell.c - get_input string qaytaradigan oxirgi versiya

#define VGA_ADDRESS 0xb8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define DIRECTORY_BUFFER ((const char *)0xA00000)




/*Kod (Hex),Inglizcha nomi,O'zbekcha nomi
0x0,Black,Qora
0x1,Blue,Ko'k
0x2,Green,Yashil
0x3,Cyan,Havorang (Zangori)
0x4,Red,Qizil
0x5,Magenta,Binafsha
0x6,Brown,Jigarrang
0x7,Light Gray,Och kulrang
0x8,Dark Gray / Bright Black,To'q kulrang (Siyohrang tusli)
0x9,Bright Blue,Och ko'k
0xA,Bright Green,Och yashil
0xB,Bright Cyan,Och havorang
0xC,Bright Red,Och qizil
0xD,Bright Magenta,Pushti (Och binafsha)
0xE,Yellow,Sariq
0xF,Bright White,Yorqin oq */
// Har bir harf va uning pozitsiyasini saqlovchi struct
typedef struct {
    char c;
    int row;
    int col;
    unsigned char color;
} TEXT;

// Harflar ro'yxatini boshqaruvchi list struktura
typedef struct {
    TEXT items[2500];
    int count;
} TextList;

// Funksiya prototiplari
unsigned char inb(unsigned short port);
char scancode_to_ascii(unsigned char scancode);
void list_clear(TextList *list);
void list_add_char(TextList *list, char c, int row, int col, unsigned char color);
void list_add_string(TextList *list, const char *str, int start_row, int start_col, unsigned char color);
void list_add_string_center(TextList *list, const char *str, int row, unsigned char color);
void screen_clear(unsigned char bg_color);
void render_frame(TextList *list, unsigned char bg_color);
int my_strlen(const char *str);
char* get_input(TextList *list, char *buffer, int max_len, int *current_row);
int my_strcmp(const char *s1, const char *s2);

// --- 1. ENTRY POINT ---
void shell_main() {
    char command_buffer[64];
    command_buffer[0] = '\0';

    // Umumiy screen_list
    TextList screen_list;
    list_clear(&screen_list);

    int current_row = 1; // Ekanning eng yuqori qismidan boshlaymiz
    list_add_string_center(&screen_list, "JETOS JETSHELL", current_row, 0x9F);
    current_row++;
    while (1) {
        // get_input ishlaydi va Enter bosilganda tayyor string qaytaradi
        char *cmd = get_input(&screen_list, command_buffer, 64, &current_row);
        if (my_strcmp(cmd, "jetos") == 0) {
                    list_add_string(&screen_list, "USHBU OS MICROSOFT DEGAN JMOTGA TEGISHLI EMAS!", current_row, 12, 0x1F);
                    current_row++;
        } else if (my_strcmp(cmd, "dirs") == 0) {
            const char *directory = DIRECTORY_BUFFER;
                list_add_string(&screen_list, "Directory:", current_row, 2, 0x1E);
                current_row++;
                list_add_string(&screen_list, directory, current_row, 4, 0x0F);
                while (*directory != '\0') {
                    if (*directory == '\n') current_row++;
                    ++directory;
                }
        }
        // Bu yerda cmd ni buyruqlar jadvali uchun tekshirishingiz mumkin
        // Masalan: if (my_strcmp(cmd, "help") == 0) { ... }

        // Keyingi sikl uchun buferni tozalaymiz
        command_buffer[0] = '\0';
    }
}

// --- 2. INPUT VA DINAMIK SCROLLING MEXANIZMI ---

char* get_input(TextList *list, char *buffer, int max_len, int *current_row) {
    int input_len = my_strlen(buffer);
    int needs_redraw = 1;

    while (1) {
        if (needs_redraw) {
            // Shu qatordagi eski harflarni tozalaymiz (harf yozganda ustma-ust chiqib qolmasligi uchun)
            int new_count = 0;
            for (int i = 0; i < list->count; i++) {
                if (list->items[i].row != *current_row) {
                    list->items[new_count++] = list->items[i];
                }
            }
            list->count = new_count;

            // Joriy qatorga "JetShell> " va yozilayotgan matnni qo'shamiz
            list_add_string(list, "JetShell> ", *current_row, 2, 0x9E);
            list_add_string(list, buffer, *current_row, 12, 0x9F);

            render_frame(list, 1);
            needs_redraw = 0;
        }

        // Klaviatradan tugma bosilishini tekshirish (Polling)
        if (inb(0x64) & 1) {
            unsigned char scancode = inb(0x60);
            
            if (!(scancode & 0x80)) {
                char c = scancode_to_ascii(scancode);
                
                if (c == '\b') { // Backspace
                    if (input_len > 0) {
                        input_len--;
                        buffer[input_len] = '\0';
                        needs_redraw = 1;
                    }
                } else if (c == '\n') { // Enter bosilganda
                    buffer[input_len] = '\0';

                    // Keyingi qatorga o'tamiz
                    (*current_row)++;

                    // Agar ekran to'lib qolsa (oxirgi qatorga yetsa)
                    if (*current_row >= VGA_HEIGHT) {
                        // Hamma elementlarning row qiymatini 1 taga kamaytiramiz (tepaga suramiz)
                        int valid_count = 0;
                        for (int i = 0; i < list->count; i++) {
                            list->items[i].row--;
                            if (list->items[i].row >= 0) {
                                list->items[valid_count++] = list->items[i];
                            }
                        }
                        list->count = valid_count;
                        *current_row = VGA_HEIGHT - 1; // Joriy qatorni oxirida ushlab turamiz
                    }

                    return buffer; // Kiritilgan stringni qaytaramiz va funksiya tugaydi
                } else if (c >= 32 && c <= 126 && input_len < max_len - 1) { // Harflar
                    buffer[input_len] = c;
                    input_len++;
                    buffer[input_len] = '\0';
                    needs_redraw = 1;
                }
            }
        }
        
        for (volatile int i = 0; i < 1000; i++);
    }
}

unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

char scancode_to_ascii(unsigned char scancode) {
    char keymap[128] = {
        0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
        '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
        0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
        0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
        '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    
    if (scancode < 128) {
        return keymap[scancode];
    }
    return 0;
}

// --- 3. YORDAMCHI UI FUNKSIYALARI ---

void list_clear(TextList *list) {
    list->count = 0;
}

void list_add_char(TextList *list, char c, int row, int col, unsigned char color) {
    if (list->count < 2500) {
        list->items[list->count].c = c;
        list->items[list->count].row = row;
        list->items[list->count].col = col;
        list->items[list->count].color = color;
        list->count++;
    }
}

void list_add_string(TextList *list, const char *str, int start_row, int start_col, unsigned char color) {
    int r = start_row;
    int c = start_col;
    while (*str) {
        if (*str == '\n') {
            r++;
            c = start_col;
        } else {
            list_add_char(list, *str, r, c, color);
            c++;
            if (c >= VGA_WIDTH) {
                c = 0;
                r++;
            }
        }
        str++;
    }
}

void list_add_string_center(TextList *list, const char *str, int row, unsigned char color) {
    int length = my_strlen(str);
    int start_col = (VGA_WIDTH - length) / 2;

    if (start_col < 0) {
        start_col = 0;
    }
    list_add_string(list, str, row, start_col, color);
}

void screen_clear(unsigned char bg_color) {
    unsigned char *video_memory = (unsigned char *) VGA_ADDRESS;
    unsigned char attribute = 0x9F;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT * 2; i += 2) {
        video_memory[i]     = ' ';
        video_memory[i + 1] = attribute;
    }
}

void render_frame(TextList *list, unsigned char bg_color) {
    screen_clear(bg_color);

    unsigned char *video_memory = (unsigned char *) VGA_ADDRESS;
    for (int i = 0; i < list->count; i++) {
        int offset = (list->items[i].row * VGA_WIDTH + list->items[i].col) * 2;
        video_memory[offset]     = list->items[i].c;
        video_memory[offset + 1] = list->items[i].color;
    }
}

int my_strlen(const char *str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}
int my_strcmp(const char *s1, const char *s2) {
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) {
            return s1[i] - s2[i];
        }
        i++;
    }
    return s1[i] - s2[i];
}