#include <cstdio>
#include <cstring>
#include <iostream>

const int kNumEmployees = 20;

class Employee {
 public:
  virtual ~Employee() = default;
  virtual void get_paid(void) = 0;
  virtual void show_income(void) {
    printf("Employee ID-%03d (%s) now has total", 
           id_,
           position_.c_str());
    if (check()) {
      printf("\033[0;33m %d\033[0;37m as income\n", total_income_);
    } else {
      printf("\033[0;31m %d\033[0;37m as income\n", total_income_);
    }
  }
  virtual bool check(void) = 0;
 protected:
  int id_;
  std::string position_;
  int total_income_;
  int num_month;
};

class Engineer: public Employee {
 public:
  Engineer(int id) {
    id_ = id;
    position_ = "Engineer";
    total_income_ = 0;
    num_month = 0;
  }
  void get_paid(void) {
    total_income_ += 20000;
    num_month += 1;
  } 
  bool check(void) {
    return total_income_ == 20000 * num_month;
  }
};

class CEO: public Employee {
 public:
  CEO(int id) {
    id_ = id;
    position_ = "CEO";
    total_income_ = 100000;
    num_month = 0;
  }
  void get_paid(void) {
    total_income_ += 250000;
    num_month += 1;
  }
  bool check(void) {
    return total_income_ == 100000 + 250000 * num_month;
  }
};

class Manager: public Employee {
 public:
  Manager(int id) {
    id_ = id;
    position_ = "Manager";
    total_income_ = 0;
    num_month = 0;
  }
  void get_paid(void) {
    total_income_ += 50000;
    num_month += 1;
  }
  bool check(void) {
    return total_income_ == 50000 * num_month;
  }
};


void setup(Employee** employees, int num_employees) {
  for (int i = 0; i < num_employees; ++i) {
    if (i == 0) {
      employees[i] = new CEO(i);
    } else if (i % 3 == 0) {
      employees[i] = new Manager(i);
    } else {
      employees[i] = new Engineer(i); 
    }
  }
}

void company_got_hacked(Employee** employees) {
  void** victim0 = (void **) employees[0];
  void** victim1 = (void **) employees[7];
  void* tmp;
  tmp = *victim0;
  *victim0 = *victim1;
  *victim1 = tmp;
}


void working_for_another_6_month(Employee** employees) {
  for (int i = 0; i < 6; ++i) {
    for (int j = 0; j < kNumEmployees; ++j) {
      employees[j]->get_paid(); 
    }
  }
}

void income_report(Employee** employees) {
  // the salary will match its position
  for (int i = 0; i < kNumEmployees; ++i) {
    employees[i]->show_income();
  }
}

void time_elapsed(void) {
  std::cout << "\n";
  std::cout << "=========================\n";
  std::cout << "  A few monthes later....\n";
  std::cout << "=========================\n\n";
}

// to check what IR looks like
void assigned_new_CEO(void) {
  CEO myceo(1337);
  myceo.get_paid();
}


int main(int argc, char** argv) {
  Employee* employees[kNumEmployees];
  setup((Employee **)&employees, kNumEmployees);
  
  // for the first {num_month} month, the income is fine
  working_for_another_6_month((Employee **) employees);
  income_report((Employee **) employees);

  // HACKED
  company_got_hacked((Employee **)employees);
  time_elapsed();

  working_for_another_6_month((Employee **) employees);
  income_report((Employee **) employees);

  return 0;
}
