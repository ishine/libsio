#ifndef SIO_LINK_H
#define SIO_LINK_H

namespace sio {

class Link {
public:
  Link();
  ~Link();

  bool  IsLinked();
  Link* Prev();
  Link* Next();
  void  InsertBefore(Link* ref);
  void  InsertAfter(Link* ref);
  void  Unlink();

private:
  Link *prev_;
  Link *next_;
};

} // namespace sio
#endif