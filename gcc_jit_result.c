struct vec3;

struct s_;

struct vec3
{
  float x;
  float y;
  float z;
  float _align;
};

struct s_
{
  struct vec3 f0;
  struct vec3 f1;
  struct vec3 f2;
  struct vec3 f3;
  float f4;
  float f5;
  float f6;
  float f7;
};

extern struct s_
function (struct s_ _arg0)
{
primary:
  return (struct s_) {.f0=_arg0.f0, .f1=_arg0.f1, .f2=_arg0.f2, .f3=_arg0.f3, .f4=_arg0.f4, .f5=_arg0.f5, .f6=_arg0.f6, .f7=_arg0.f7};
}

