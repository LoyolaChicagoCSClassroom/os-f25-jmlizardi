# MMU Testing Guide

## **Testing Method 1: Using GDB with Custom Script**

### **Step 1: Build the kernel**
```bash
make clean && make
```

### **Step 2: Start debugging session**
```bash
# Use the custom MMU testing GDB script
screen -S qemu -d -m qemu-system-i386 -S -s -hda rootfs.img
TERM=xterm i386-unknown-elf-gdb -x gdb_mmu_test.txt
```

### **Step 3: Debug through MMU setup**
In GDB, you can:

1. **Step through MMU initialization:**
   ```gdb
   # Step to identity_map_kernel function
   (gdb) n
   (gdb) dump_page_directory
   (gdb) dump_page_table
   ```

2. **Check page directory loading:**
   ```gdb
   # Step to loadPageDirectory
   (gdb) n
   (gdb) check_cr3
   ```

3. **Verify paging enablement:**
   ```gdb
   # Step to enable_paging
   (gdb) n
   (gdb) check_paging_enabled
   ```

4. **Test virtual memory mapping:**
   ```gdb
   # After paging is enabled
   (gdb) test_virtual_memory
   ```

## **Testing Method 2: Using QEMU Monitor**

### **Step 1: Start with monitor**
```bash
make debug-monitor
```

### **Step 2: In separate terminal, attach to QEMU monitor**
```bash
screen -r qemu
```

### **Step 3: Use QEMU monitor commands**
In the QEMU monitor:
```
(qemu) info mem        # Show page table mappings
(qemu) info registers  # Show CPU registers including CR3
(qemu) x/10i $pc      # Show current instructions
```

## **What to Look For**

### **Success Indicators:**
1. **No system reset** after `enable_paging()`
2. **Page directory entries show present=1** for mapped pages
3. **Page table entries show correct frame addresses**
4. **Virtual memory test writes/reads successfully**
5. **QEMU monitor shows proper mappings with `info mem`**

### **Failure Indicators:**
1. **System resets** immediately after enabling paging
2. **Triple fault** in QEMU
3. **Page fault exceptions**
4. **Incorrect frame addresses** in page tables

## **Common Issues and Debugging**

### **Issue 1: System resets after enabling paging**
**Cause:** Page tables not properly set up
**Debug:**
```gdb
(gdb) dump_page_directory
(gdb) dump_page_table
# Check if identity mappings are correct
```

### **Issue 2: Page fault on instruction fetch**
**Cause:** Kernel code not properly identity mapped
**Debug:**
```gdb
(gdb) info registers
# Check if EIP is in mapped range
```

### **Issue 3: Stack access fails**
**Cause:** Stack not properly identity mapped
**Debug:**
```gdb
(gdb) print $esp
# Verify stack is in mapped range
```

## **Manual Testing Steps**

### **Step 1: Verify Page Directory Setup**
```gdb
(gdb) b identity_map_kernel
(gdb) c
(gdb) dump_page_directory
# Should show at least pd[0].present = 1
```

### **Step 2: Verify Page Table Entries**
```gdb
(gdb) dump_page_table
# Should show entries for:
# - 0x100000 (kernel start)
# - Stack pages
# - 0xB8000 (VGA buffer)
```

### **Step 3: Test Paging Enable**
```gdb
(gdb) b enable_paging
(gdb) c
(gdb) n  # Execute enable_paging()
# If system doesn't crash, paging works!
```

### **Step 4: Test Virtual Memory Access**
```gdb
(gdb) b map_pages
(gdb) c
# Step through the virtual memory test
(gdb) n
# Verify no page faults occur
```

## **Expected Output in GDB**

When working correctly, you should see:
```
=== Setting up MMU and enabling paging ===
Identity mapping kernel: 0x00100000 to 0x00105000
Identity mapping stack around: 0x0009F000
Identity mapping video buffer: 0x000B8000
Identity mapping complete.
Loading page directory into CR3...
Enabling paging...
Paging enabled successfully! Virtual memory is now active.
```

## **QEMU Monitor Commands**

Useful QEMU monitor commands:
```
info mem                 # Page table dump
info registers          # CPU registers (including CR3)
info mtree              # Memory tree
x/10x 0x100000          # Examine kernel memory
```

This comprehensive testing approach will help you verify that your MMU implementation is working correctly!
