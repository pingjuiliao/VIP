#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

class Account {
 public:
  virtual ~Account() = default;
  virtual void operate(void) = 0;
 protected:
  int id_;
};

class User: public Account {
 public:
  User(int id) { id_ = id; }
  virtual ~User() = default;
  virtual void operate(void) {
    printf("User (Id: %2d) has made an operation\n", id_);
  }
};

class Admin: public Account {
 public:
  Admin(int id) { id_ = id; }
  virtual ~Admin() = default;
  virtual void operate(void) {
    printf("Admin (Id: %2d) is making operations\n", id_);
  }
};


int main(int argc, char** argv) {
  Admin* admin = nullptr;
  admin = new Admin(0);
  printf("admin has registed at %p\n\n", admin);

  User* user = nullptr; 
  user = new User(1);
  printf("user has registered at %p\n\n", user); 
  // vuln
  memcpy((void *)user, (void *)admin, sizeof(User));
  user->operate();

  return 0;
}
