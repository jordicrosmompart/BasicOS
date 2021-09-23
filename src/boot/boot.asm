ORG 0x7c00 ; sets origin to 7c00 (bootloader sector in real mode)
BITS 16 ; 16bit real mode code follows

CODE_SEG equ gdt_code - gdt_start ; offset of the code segment of the GDT
DATA_SEG equ gdt_data - gdt_start ; offset of the data segment of the GDT

jmp short start
nop

; FAT16 header
OEMIdentifier       db 'CROSOS  ' ; 8 bytes required
BytesPerSector      dw 0x200 ; 512 bytes per sector
SectorsPerCluster   db 0x80
ReservedSectors     dw 200 ; Reserved space for the kernel
FATCopies           db 0x02
RootDirEntries      dw 0x40
NumSectors          dw 0x00 ; not used
MediaType           db 0xF8
SectorsPerFat       dw 0x100
SectorsPerTrack     dw 0x20
NumberOfHeads       dw 0x40
HiddenSectors       dd 0x00
SectorsBig          dd 0x773594

; Extended BPB (FAT16)
DriveNumber         db 0x80
WinNTBit            db 0x00
Signature           db 0x29
VolumeID            dd 0xD105
VolumeIDString      db 'CROSOS BOOT' ; 11 bytes required
SystemIDString      db 'FAT16   ' ; 8 bytes required


 
start:
    jmp 0:step2 ; jump to cs = 0 and ip = step2 --> jump to step2 replacing cs to 0

step2:
    cli ; Clear Interrupts
    mov ax, 0x00 ; set ds, es, ss, sp to 0
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00 ; set sp to 7c00
    ; this places all the segment registers to 0 (+ origin 7c00) and the stack pointer to 7c00 (+ origin)
    sti ; Enables Interrupts

.load_protected:
    cli
    lgdt[gdt_descriptor] ; command necessary to load the GDT
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax ; enable protected mode
    ; once in protected mode, the CS/DS etc. registers act like an offset to look on the GDT table, they work completely different to real mode
    jmp CODE_SEG:load32 ; set cs to the code_seg offset in the GDT table (8)

; GDT
gdt_start:
gdt_null:
    dd 0x0
    dd 0x0

; offset 0x8
; This GDT creates only a segment of 2GB of executable code and 2GB of data
gdt_code:     ; CS SHOULD POINT TO THIS
    dw 0xffff ; Segment limit first 0-15 bits 
    dw 0      ; Base first 0-15 bits
    db 0      ; Base 16-23 bits
    db 0x9a   ; Access byte
    db 11001111b ; High 4 bit flags and the low 4 bit flags
    db 0        ; Base 24-31 bits

; offset 0x10
gdt_data:      ; DS, SS, ES, FS, GS
    dw 0xffff ; Segment limit first 0-15 bits
    dw 0      ; Base first 0-15 bits
    db 0      ; Base 16-23 bits
    db 0x92   ; Access byte
    db 11001111b ; High 4 bit flags and the low 4 bit flags
    db 0        ; Base 24-31 bits

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start-1
    dd gdt_start

[BITS 32]
load32:
    mov eax, 1 ; Starting sector we wanna load from (0 is the boot sector)
    mov ecx, 100 ; Number of sectors we want to load
    mov edi, 0x0100000 ;1MB, the address we want to load the sectors to
    call ata_lba_read
    jmp CODE_SEG:0x0100000 ; call kernel start function. 

ata_lba_read:
    mov ebx, eax ; Backup the LBA
    shr eax, 24 ; Shift 24 bits to the right -> send higest 8 bits to hard disk controller
    or eax, 0xE0 ; Select the master drive
    mov dx, 0x1F6 ; Port 
    out dx, al ; out sends the value in al to port dx
    ; Finished sending the highest 8 bits of the LBA

    ; Send the total sectors to read
    mov eax, ecx
    mov dx, 0x1F2
    out dx, al
    ; Finished the total sectors to read

    ; Sending first bits of the LBA
    mov eax, ebx ; Restore the backup LBA
    mov dx, 0x1F3
    out dx, al
    ; Finished sending more bits of the LBA

    ; Sending middle bits of the LBA
    mov dx, 0x1F4
    mov eax, ebx ; Restore backup LBA
    shr eax, 8
    out dx, al
    ; Finished sending more bits of the LBA

    ; Send upper bits of the LBA
    mov dx, 0x1F5
    mov eax, ebx ; Restore backup LBA
    shr eax, 16
    out dx, al
    ; Finished sending upper 16 bits of the LBA

    ; Command port of disk controller
    mov dx, 0x1F7 
    mov al, 0x20 ; read sectors with retry
    out dx, al

    ; Read all sectors into memory
.next_sector:
    push ecx

; Checking if we need to read
.try_again:
    mov dx, 0x1F7
    in al, dx
    test al, 8
    jz .try_again

; We need to read 256 words at a time
    mov ecx, 256
    mov dx, 0x1F0
    rep insw ; Reads a word from I/O port 0x1F0 and store it into 0x0100000 (edi)
    pop ecx
    loop .next_sector ; decrements automatically ecx
    ; End of reading sectors into memory
    ret

times 510-($ - $$) db 0
dw 0xAA55