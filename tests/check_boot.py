"""Boot the GRUB ISO and inspect physical VGA, GDTR and saved stack words.

Run with: python3 tests/check_boot.py ISO ELF OUTPUT_DIRECTORY
The stack is reused by VGA calls: compare the preserved snapshot, not live ESP.
"""
import subprocess, time, re, pathlib, sys
iso,elf,out=sys.argv[1:]; out=pathlib.Path(out); out.mkdir(parents=True,exist_ok=True)
p=subprocess.Popen(['qemu-system-i386','-cdrom',iso,'-display','none','-no-reboot','-no-shutdown','-monitor','stdio'],stdin=subprocess.PIPE,stdout=open(out/'monitor.txt','w'),stderr=open(out/'qemu.log','w'))
try:
 def cmd(c):
  p.stdin.write((c+'\n').encode()); p.stdin.flush(); time.sleep(.3)
  return (out/'monitor.txt').read_text()
 time.sleep(2); cmd('sendkey ret'); time.sleep(2)
 regs=cmd('info registers'); (out/'registers.txt').write_text(regs)
 cmd('stop'); cmd('pmemsave 0xb8000 4000 '+'"'+str(out/'vga.bin')+'"')
 raw=(out/'vga.bin').read_bytes(); rows=[''.join(chr(raw[2*(r*80+c)]) for c in range(80)).rstrip() for r in range(25)]
 (out/'screen.txt').write_text('\n'.join(rows)+'\n')
 assert 'KERNEL STACK DUMP' in rows[0],rows
 esp=int(rows[3].split(':')[1],16)
 bottom=int(rows[1].split(':')[1],16); top=int(rows[2].split(':')[1],16)
 assert bottom<=esp and esp+20<=top and esp%4==0
 symbols=subprocess.check_output(['nm','-a',elf],text=True)
 snapshot=int(next(line.split()[0] for line in symbols.splitlines() if re.search(r' snapshot(?:\.\d+)?$',line)),16)
 cmd('pmemsave '+hex(snapshot)+' 20 "'+str(out/'snapshot.bin')+'"')
 captured=(out/'snapshot.bin').read_bytes()
 for i in range(5):
  a,v=rows[6+i].split(); assert int(a,16)==esp+4*i
  assert int.from_bytes(captured[4*i:4*i+4],'little')==int(v,16)
 assert re.search(r'GDT=\s*00000800\s+00000037',regs),regs
 for seg,val in [('CS','0008'),('DS','0010'),('SS','0018')]: assert re.search(seg+' ='+val,regs),regs
 cmd('pmemsave 0x800 56 '+'"'+str(out/'gdt.bin')+'"')
 # The CPU may set descriptor access-byte bit 0 when loading a segment.
 descriptors=(out/'gdt.bin').read_bytes()
 expected=[0,0x00CF9A000000FFFF,0x00CF92000000FFFF,0x00CF92000000FFFF,0x00CFFA000000FFFF,0x00CFF2000000FFFF,0x00CFF2000000FFFF]
 for i, value in enumerate(expected):
  actual=int.from_bytes(descriptors[8*i:8*i+8], 'little')
  assert actual & ~(1 << 40) == value, (i, hex(actual))
 (out/'registers.txt').write_text(re.sub(r'\x1b\[[0-9;]*[A-Za-z]', '', regs))
 print('PASS',iso,'GDTR, segments, 7 descriptors, 5 stack pairs',hex(esp))
finally:
 p.terminate(); p.wait()
