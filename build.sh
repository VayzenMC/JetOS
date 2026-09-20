#!/bin/bash
set -e

set -e

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"
ISO_DIR="$BUILD_DIR/iso"
DIST_DIR="$ROOT_DIR/dist"

echo "JetOS va Shell build boshlandi..."

# rm -rf "$BUILD_DIR" "$DIST_DIR"
mkdir -p "$BUILD_DIR" "$ISO_DIR/boot/grub" "$DIST_DIR"

# Kernel va ISO9660 CD reader.
nasm -f elf32 "$ROOT_DIR/src/kernel/boot.asm" -o "$BUILD_DIR/boot.o"
nasm -f elf32 "$ROOT_DIR/src/kernel/interrupts.asm" -o "$BUILD_DIR/interrupts.o"
gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdinc \
    -c "$ROOT_DIR/src/kernel/kernel.c" -o "$BUILD_DIR/kernel.o"
gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdinc \
    -c "$ROOT_DIR/src/kernel/vm.c" -o "$BUILD_DIR/vm.o"
gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdinc \
    -c "$ROOT_DIR/src/kernel/interrupts.c" -o "$BUILD_DIR/interrupts.o.c"
gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdinc \
    -c "$ROOT_DIR/src/kernel/iso9660.c" -o "$BUILD_DIR/iso9660.o"
g++ -m32 -ffreestanding -fno-pie -fno-stack-protector -fno-exceptions -fno-rtti \
    -nostdinc -nostdinc++ -I"$ROOT_DIR/src" -c "$ROOT_DIR/src/filemanagerdriver.cpp" \
    -o "$BUILD_DIR/filemanagerdriver.o"

# Shellni standalone binaryga aylantirish.
gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdinc \
    -I"$ROOT_DIR/src" -c "$ROOT_DIR/src/shell/shell.c" -o "$BUILD_DIR/shell.o"
ld -m elf_i386 -T "$ROOT_DIR/src/shell/link.ld" "$BUILD_DIR/shell.o" \
    -o "$BUILD_DIR/shell.elf"
objcopy -O binary "$BUILD_DIR/shell.elf" "$BUILD_DIR/shell.bin"

ld -m elf_i386 -T "$ROOT_DIR/src/kernel/link.ld" \
    "$BUILD_DIR/boot.o" "$BUILD_DIR/interrupts.o" "$BUILD_DIR/kernel.o" \
    "$BUILD_DIR/vm.o" "$BUILD_DIR/interrupts.o.c" "$BUILD_DIR/iso9660.o" \
    "$BUILD_DIR/filemanagerdriver.o" \
    -o "$BUILD_DIR/kernel.bin"

# ISO ichidagi shell fayli kernel tomonidan CD filesystem'dan o'qiladi.
cp "$BUILD_DIR/kernel.bin" "$ISO_DIR/boot/kernel.bin"
cp "$BUILD_DIR/shell.bin" "$ISO_DIR/boot/shell.bin"

cat << 'EOF' > "$ISO_DIR/boot/grub/grub.cfg"
set timeout=0
set default=0

menuentry "JetOS" {
    multiboot /boot/kernel.bin
    boot
}
EOF

grub-mkrescue -o "$DIST_DIR/jetos.iso" "$ISO_DIR"

echo "Tayyor! dist/jetos.iso yaratildi."