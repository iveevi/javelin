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
  bool f6;
  bool f7;
};

extern float
function (struct s_ _arg0, struct vec3 _arg1, struct vec3 _arg2)
{
primary:
  return _arg0.f5 * _arg0.f5 / (float)3.141593 * powf (((float)cos (((double)(float)acos (((double)clamp ((_arg1.x * _arg2.x + _arg1.y * _arg2.y + _arg1.z * _arg2.z), (float)0.000000, (float)0.999000)))))), (float)4.000000) * powf ((_arg0.f5 * _arg0.f5 + (float)tan (((double)(float)acos (((double)clamp ((_arg1.x * _arg2.x + _arg1.y * _arg2.y + _arg1.z * _arg2.z), (float)0.000000, (float)0.999000))))) * (float)tan (((double)(float)acos (((double)clamp ((_arg1.x * _arg2.x + _arg1.y * _arg2.y + _arg1.z * _arg2.z), (float)0.000000, (float)0.999000)))))), (float)2.000000);
}

extern float
clamp (float x, float low, float high); /* (imported) */

