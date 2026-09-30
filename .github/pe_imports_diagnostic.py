import os,struct,sys
from pathlib import Path
from functools import lru_cache

class PE:
 def __init__(self,path):
  self.path=path;self.data=path.read_bytes();nt=self.u32(0x3c);n=self.u16(nt+6);opt=nt+24
  self.bits=64 if self.u16(opt)==0x20b else 32
  dirs=opt+(112 if self.bits==64 else 96)
  self.export=self.u32(dirs);self.imp=self.u32(dirs+8)
  off=opt+self.u16(nt+20)
  self.sections=[(self.u32(off+i*40+12),self.u32(off+i*40+8),self.u32(off+i*40+20),self.u32(off+i*40+16)) for i in range(n)]
 def u16(self,o):return struct.unpack_from('<H',self.data,o)[0]
 def u32(self,o):return struct.unpack_from('<I',self.data,o)[0]
 def offset(self,rva):
  for va,size,off,raw in self.sections:
   if va<=rva<va+max(size,raw):return off+rva-va
  return rva
 def string(self,rva):
  o=self.offset(rva);return self.data[o:self.data.index(b'\0',o)].decode(errors='replace')
 def imports(self):
  if not self.imp:return
  off=self.offset(self.imp)
  while self.u32(off+12):
   dll=self.string(self.u32(off+12));thunk=self.offset(self.u32(off) or self.u32(off+16));names=[]
   while True:
    value=struct.unpack_from('<Q' if self.bits==64 else '<I',self.data,thunk)[0]
    if not value:break
    if not value>>(self.bits-1):names.append(self.string(value+2))
    thunk+=self.bits//8
   yield dll,names
   off+=20
 def exports(self):
  if not self.export:return set()
  off=self.offset(self.export);count=self.u32(off+24);names=self.offset(self.u32(off+32))
  return {self.string(self.u32(names+i*4)) for i in range(count)}

exe=Path(sys.argv[1]);dirs=[exe.parent]+[Path(p) for p in sys.argv[2:]]
index={}
for d in dirs:
 if d.is_dir():
  for f in d.glob('*.dll'):index.setdefault(f.name.lower(),f)
seen=set();missing=[]
@lru_cache(None)
def read(p):return PE(p)
def visit(p):
 if p in seen:return
 seen.add(p)
 for dll,names in read(p).imports():
  target=index.get(dll.lower())
  if not target:
   if not dll.lower().startswith(('api-ms-','ext-ms-')):print('UNRESOLVED DLL',p.name,dll)
   continue
  for name in names:
   if name not in read(target).exports():
    print('MISSING EXPORT',p.name,'imports',name,'from',target);missing.append((p.name,dll,name))
  visit(target)
visit(exe)
print('Inspected',len(seen),'PE files;',len(missing),'missing named exports')
