import os, subprocess, re
from SCons.Script import File
Import("env")

img_file      = "bareOS.elf"
HEAD_DIR = Dir('load').abspath

def error(code, msg):
    print(msg)
    Exit(code)
# auto get src directories
kernel_dirs = [
    d
    for d in os.listdir("kernel")
    if os.path.isdir(os.path.join("kernel", d)) and d != "include"
]
kernel_files  = [Glob(os.path.join("kernel", d, "*.[cs]")) for d in kernel_dirs]
def do_gdb(target, source, env):
    gdb_file = img_file
    os.system(f'{env["GDB"]} -ex "file {gdb_file}" -ex "target remote :{env["port"]}"')
    env.Exit(0)

bld_gdb = env.Builder(action=do_gdb)
env["BUILDERS"]["GDB"] = bld_gdb

def do_qemu(target, source, env):
    env['qflags'] += f" -S -gdb tcp::{env['port']}"

bld_qemu = env.Builder(action=do_qemu)
env["BUILDERS"]["Qemu"] = bld_qemu

# ----------------------  Build OS Image  ----------------------------
objs   = env.Object(kernel_files)
elf    = env.Program(img_file, objs)
elf    = env.Command(None, elf, Copy(img_file, "$SOURCE"))
env.Alias("gdb", objs)

# ------------------- Do External File Load  -------------------------
def prepare_generic_loader(env):
    # Figure out heap start address based on the memmap. 
    START_ADDR = 24 # sizeof(alloc_t)
    try:
        with open(str(env["memmap"])) as mf:
            heap_addr = None
            for line in mf:
                # look for the mem_start address after the compiler spits out a memmap
                m = re.search(r"(0x[0-9a-fA-F]+)\s+PROVIDE \(mem_start", line)
                if m:
                    heap_addr = int(m.group(1), 16)
                    break
        if heap_addr is not None:
            print(f"[Info] Predicted heap start (mem_start): 0x{heap_addr:08x}")
            START_ADDR += heap_addr
        else:
            print("[Warning] Could not locate mem_start in linker map; skipping file load.")
    except Exception as e:
        print(f"[Warning] Failed to read linker map for heap prediction: {e}")

    # Get file load constants from the fs.h constants that defines the ramdisk. If ramdisk updates, so do these limits. 
    fs_header = File('#/kernel/include/fs/fs.h').abspath
    with open(fs_header) as f:
        fs_src = f.read()

    def _const(name):
        m = re.search(r"#define\s+%s\s+(\d+)" % name, fs_src)
        if not m:
            raise ValueError(f"[Warning ]{name} not found in fs.h")
        return int(m.group(1))

    INODE_BLOCKS = _const("INODE_BLOCKS")
    BLOCK_SIZE = _const("MDEV_BLOCK_SIZE")
    FILENAME_LEN = _const("FILENAME_LEN")
    DIR_SIZE = _const("DIR_SIZE")
    MAX_FILE_SIZE = INODE_BLOCKS * BLOCK_SIZE
    HEADER_SIZE = FILENAME_LEN + 16

    def do_name_bytes(stem: str) -> bytes:
        b = stem.encode('utf-8')
        if len(stem) > FILENAME_LEN - 1: # account for null terminator fs needs
            return None
        return b[:FILENAME_LEN].ljust(FILENAME_LEN, b'\0')

    def do_size_bytes(size: int) -> bytes:
        return int(size).to_bytes(16, 'little')

    #Load user filess from /load, this is attached to qflags
    load_flags = ""

    if START_ADDR > 24:
        current = START_ADDR + 1
        LOAD_DIR = Dir('#/load').abspath
        if not os.path.exists(LOAD_DIR):
            os.makedirs(LOAD_DIR)
        if not os.path.exists(HEAD_DIR):
            os.makedirs(HEAD_DIR)
        files = sorted(
            f for f in os.listdir(LOAD_DIR)
            if os.path.isfile(os.path.join(LOAD_DIR, f))
        )

        total_bytes = 0
        valid_files = 0

        for f in files:
            if valid_files == DIR_SIZE:
                break
            stem = os.path.splitext(f)[0]
            name_bytes = do_name_bytes(stem)
            if name_bytes is None:
                continue

            path = os.path.join(LOAD_DIR, f)
            f_size = os.path.getsize(path)
            if f_size > MAX_FILE_SIZE:
                continue

            header_path = os.path.join(HEAD_DIR, stem + ".gih")

            # Force rewrite header.
            header = name_bytes + do_size_bytes(f_size)
            assert len(header) == HEADER_SIZE
            with open(header_path, 'wb') as hf:
                hf.write(header)

            # load header
            load_flags += f"-device loader,file={header_path},addr=0x{current:08x} "
            current += HEADER_SIZE

            # load file
            load_flags += f"-device loader,file={path},addr=0x{current:08x} "
            current += f_size

            total_bytes += HEADER_SIZE + f_size
            valid_files += 1
        load_flags += f"-device loader,addr={START_ADDR:#x},data={valid_files:#x},data-len=1 "
        return load_flags

env.Alias("build", elf)
# ----------------------  Virtualization  ----------------------------
if "debug" in COMMAND_LINE_TARGETS:
    env['qflags'] += f" -S -gdb tcp::{env['port']}"

def run_qemu(target, source, env):
    # This has to happen immediately prior to QEMU spinup so the helper can read the memmap.
    load_flags = prepare_generic_loader(env)
    print(env['qflags'])
    if load_flags:
        env.Append(qflags=" " + load_flags)
        print(load_flags)

    args = [env['QEMU'], '-kernel', str(source[0])] + env['qflags'].split()
    return subprocess.call(args)

run = env.Command(target="run_cmd", source=img_file, action=run_qemu)
env.Depends(run, elf)
env.Alias("run", run)

# --------------------  Environment Management  ----------------------

env.Clean(elf, env["memmap"])
env.Clean(elf, HEAD_DIR)
env.Clean(elf, f"#{img_file}")
