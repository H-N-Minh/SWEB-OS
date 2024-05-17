#pragma once

class IPTEntry {
public:
  int ppn;
  int vpn;
  int pid;
  bool valid;

  IPTEntry(int ppn, int vpn, int pid) : ppn(ppn), vpn(vpn), pid(pid), valid(true) {}
};
