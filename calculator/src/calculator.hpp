#ifndef CALCULATOR_HPP
#define CALCULATOR_HPP

template <typename T>
class Calculator {
 public:
  Calculator(T a, T b) : a_(a), b_(b) {}

  T add() const { return a_ + b_; }
  T subtract() const { return a_ - b_; }
  T multiply() const { return a_ * b_; }

  T divide() const {
    if (b_ == 0) {
      return static_cast<T>(0);  // não lança exceção
    }
    return a_ / b_;
  }

 private:
  T a_;
  T b_;
};

#endif
