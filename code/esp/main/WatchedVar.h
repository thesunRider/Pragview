class WatchedVar {
  int value;
  void (*onChange)(int);

public:
  WatchedVar(int initial = 0, void (*cb)(int) = nullptr) {
    value = initial;
    onChange = cb;
  }

  void set(int v) {
    if (v != value) {
      value = v;
      if (onChange) onChange(value);
    }
  }

  int get() { return value; }
};
