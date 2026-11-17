/* Adapter for the two distinct buffers emitted by this pinned Croissant ABI.
 * The ABI contains sizes, not pointers; reject any other allocation or size reuse.
 * A general native ArrayBuffer representation belongs in Croissant.
 */
#include "croissant/intrinsics.h"
#define ARENA_SIZE (64u * 1024u * 1024u)
static uint8_t arena[ARENA_SIZE] __attribute__((aligned(16), section(".arena")));
static uint8_t scratch[8] __attribute__((aligned(8)));
static bool arena_allocated, scratch_allocated;
extern void rei_panic(const char *);

void *memset(void *p, int value, size_t size) {
    uint8_t *q = p; for (size_t i = 0; i < size; ++i) q[i] = (uint8_t)value; return p;
}
void *memcpy(void *p, const void *s, size_t size) {
    uint8_t *d = p; const uint8_t *q = s; for (size_t i = 0; i < size; ++i) d[i] = q[i]; return p;
}
void *memmove(void *p, const void *s, size_t size) {
    uint8_t *d = p; const uint8_t *q = s;
    if ((uintptr_t)d <= (uintptr_t)q) { for (size_t i = 0; i < size; ++i) d[i] = q[i]; }
    else { while (size) { --size; d[size] = q[size]; } } return p;
}
__attribute__((noreturn)) void __bake_c_abort_range(void) {
    rei_panic("REI: INTRINSIC RANGE OR UNSUPPORTED BUFFER\n");
    for (;;) __asm__ volatile("wfe");
}
static uint8_t *base(__bake_c_Buffer b) {
    if (b.byteLength == 8 && scratch_allocated) return scratch;
    if (b.byteLength == ARENA_SIZE && arena_allocated) return arena;
    __bake_c_abort_range();
}
static uint8_t *at(__bake_c_Buffer b, double offset, size_t n) {
    if (!(offset >= 0 && offset <= b.byteLength && (double)n <= b.byteLength - offset)) __bake_c_abort_range();
    size_t k = (size_t)offset;
    if ((double)k != offset) __bake_c_abort_range();
    return base(b) + k;
}
__bake_c_Buffer __bake_c_new_buffer(double size) {
    if (size == 8 && !scratch_allocated) { scratch_allocated = true; memset(scratch, 0, 8); }
    else if (size == ARENA_SIZE && !arena_allocated) { arena_allocated = true; memset(arena, 0, ARENA_SIZE); }
    else __bake_c_abort_range();
    return (__bake_c_Buffer){.byteLength = size};
}
__bake_c_Buffer __bake_c_new_shared_buffer(double size) { return __bake_c_new_buffer(size); }
__bake_c_View __bake_c_new_view(__bake_c_Buffer b) { return (__bake_c_View){.buffer=b}; }
__bake_c_U8Array __bake_c_new_u8_array(__bake_c_Buffer b) { return (__bake_c_U8Array){.buffer=b,.length=b.byteLength}; }
__bake_c_I32Array __bake_c_new_i32_array(__bake_c_Buffer b) { return (__bake_c_I32Array){.buffer=b,.length=b.byteLength/4}; }
__bake_c_U64Array __bake_c_new_u64_array(__bake_c_Buffer b) { return (__bake_c_U64Array){.buffer=b,.length=b.byteLength/8}; }
static uint64_t read_bits(__bake_c_Buffer b, double offset, unsigned n, bool le) {
    const uint8_t *p=at(b,offset,n); uint64_t result=0;
    for(unsigned i=0;i<n;i++) result |= (uint64_t)p[i] << (8*(le?i:n-1-i));
    return result;
}
static void write_bits(__bake_c_Buffer b,double offset,unsigned n,bool le,uint64_t value) {
    uint8_t *p=at(b,offset,n);
    for(unsigned i=0;i<n;i++) p[i]=(uint8_t)(value>>(8*(le?i:n-1-i)));
}
static uint64_t bits(double x) { union {double f; uint64_t u;} v={.f=x};return v.u; }
static double value(uint64_t x) { union {double f; uint64_t u;} v={.u=x};return v.f; }
static uint32_t u32(double x) {
    uint64_t raw=bits(x), exp=(raw>>52)&2047u;
    if (exp<1023 || exp>=1087) return 0;
    unsigned e=(unsigned)(exp-1023);
    uint64_t mantissa=(raw&UINT64_C(0xfffffffffffff))|UINT64_C(0x10000000000000);
    uint32_t result=(uint32_t)(e>=52?mantissa<<(e-52):mantissa>>(52-e));
    return raw>>63?0u-result:result;
}
int32_t croissant_js_and(double a,double b){return (int32_t)(u32(a)&u32(b));}
int32_t croissant_js_or(double a,double b){return (int32_t)(u32(a)|u32(b));}
int32_t croissant_js_xor(double a,double b){return (int32_t)(u32(a)^u32(b));}
int32_t croissant_js_shl(double a,double b){return (int32_t)(u32(a)<<(u32(b)&31));}
int32_t croissant_js_shr(double a,double b){return (int32_t)u32(a)>>(u32(b)&31);}
uint32_t croissant_js_ushr(double a,double b){return u32(a)>>(u32(b)&31);}
int32_t croissant_js_not(double a){return (int32_t)~u32(a);}
/* The only emitted imul call is immediately coerced to uint32_t. Preserve
 * that result without an out-of-range negative floating-to-unsigned C cast. */
double __bake_c_i32_mul(double a,double b){return (double)(u32(a)*u32(b));}
double __bake_c_f64_trunc(double x){double y;__asm__("frintz %d0, %d1":"=w"(y):"w"(x));return y;}
double __bake_c_f64_floor(double x){double y;__asm__("frintm %d0, %d1":"=w"(y):"w"(x));return y;}
double __bake_c_f64_ceil(double x){double y;__asm__("frintp %d0, %d1":"=w"(y):"w"(x));return y;}
double __bake_c_f64_sqrt(double x){double y;__asm__("fsqrt %d0, %d1":"=w"(y):"w"(x));return y;}
double __bake_c_f64_min(double a,double b){double y;__asm__("fmin %d0, %d1, %d2":"=w"(y):"w"(a),"w"(b));return y;}
double __bake_c_f64_max(double a,double b){double y;__asm__("fmax %d0, %d1, %d2":"=w"(y):"w"(a),"w"(b));return y;}
bool __bake_c_f64_is_nan(double x){return x!=x;}
bool __bake_c_f64_is_integer(double x){return (bits(x)&UINT64_C(0x7ff0000000000000))!=UINT64_C(0x7ff0000000000000)&&x==__bake_c_f64_trunc(x);}
double __bake_c_f32_round(double x){return (double)(float)x;}
double croissant_js_remainder(double a,double b){
    const uint64_t mask=UINT64_C(0x7fffffffffffffff), inf=UINT64_C(0x7ff0000000000000);
    uint64_t aa=bits(a)&mask, bb=bits(b)&mask;
    if(aa>=inf||bb>inf||bb==0)return value(UINT64_C(0x7ff8000000000000));
    double x=value(aa), y=value(bb);
    if(x<y)return a;
    double scaled=y;
    while(scaled<=x/2)scaled*=2;
    while(scaled>=y){if(x>=scaled)x-=scaled;if(scaled==y)break;scaled*=0.5;}
    return (bits(a)>>63)?-x:x;
}
uint64_t __bake_c_as_uint_n(double n,uint64_t x){if(n==64)return x;if(n<0||n>64||n!=__bake_c_f64_trunc(n))__bake_c_abort_range();return x&((UINT64_C(1)<<(unsigned)n)-1);}
uint64_t __bake_c_as_int_n(double n,uint64_t x){uint64_t y=__bake_c_as_uint_n(n,x);if(n==0||n==64)return y;uint64_t s=UINT64_C(1)<<((unsigned)n-1);return (y^s)-s;}
uint64_t __bake_c_bigint(double x){if(!__bake_c_f64_is_integer(x)||x < -0x1p63 || x>=0x1p64)__bake_c_abort_range();return x<0?(uint64_t)(int64_t)x:(uint64_t)x;}
double __bake_c_number(double x){return x;}
double __bake_c_u8_get(__bake_c_U8Array a,double i){return *at(a.buffer,i,1);}
double __bake_c_u8_set(__bake_c_U8Array a,double i,double x){uint8_t v=(uint8_t)u32(x);*at(a.buffer,i,1)=v;return v;}
static size_t clamp(double x,double length){if(x!=x)return 0;x=__bake_c_f64_trunc(x);if(x<0)x+=length;if(x<=0)return 0;if(x>=length)return (size_t)length;return (size_t)x;}
__bake_c_U8Array __bake_c_u8_fill(__bake_c_U8Array a,double v,double start,double end){size_t s=clamp(start,a.length),e=clamp(end,a.length);if(e>s)memset(at(a.buffer,s,e-s),(int)u32(v),e-s);return a;}
__bake_c_U8Array __bake_c_u8_copy_within(__bake_c_U8Array a,double target,double start,double end){size_t t=clamp(target,a.length),s=clamp(start,a.length),e=clamp(end,a.length);size_t n=e>s?e-s:0;if(n>a.length-t)n=(size_t)a.length-t;if(n)memmove(at(a.buffer,t,n),at(a.buffer,s,n),n);return a;}
double __bake_c_i32_get(__bake_c_I32Array a,double i){return (int32_t)read_bits(a.buffer,i*4,4,true);}
double __bake_c_i32_set(__bake_c_I32Array a,double i,double v){write_bits(a.buffer,i*4,4,true,u32(v));return (int32_t)u32(v);}
static int32_t *atomic32(__bake_c_I32Array a,double i){return (int32_t *)(void *)at(a.buffer,i*4,4);}
static uint64_t *atomic64(__bake_c_U64Array a,double i){return (uint64_t *)(void *)at(a.buffer,i*8,8);}
double __bake_c_atomic_add_i32(__bake_c_I32Array a,double i,double v){return __atomic_fetch_add(atomic32(a,i),(int32_t)u32(v),__ATOMIC_SEQ_CST);}
double __bake_c_atomic_cmpxchg_i32(__bake_c_I32Array a,double i,double e,double v){int32_t old=(int32_t)u32(e);__atomic_compare_exchange_n(atomic32(a,i),&old,(int32_t)u32(v),false,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);return old;}
double __bake_c_atomic_load_i32(__bake_c_I32Array a,double i){return __atomic_load_n(atomic32(a,i),__ATOMIC_SEQ_CST);}
double __bake_c_atomic_store_i32(__bake_c_I32Array a,double i,double v){int32_t x=(int32_t)u32(v);__atomic_store_n(atomic32(a,i),x,__ATOMIC_SEQ_CST);return x;}
uint64_t __bake_c_atomic_add_u64(__bake_c_U64Array a,double i,uint64_t v){return __atomic_fetch_add(atomic64(a,i),v,__ATOMIC_SEQ_CST);}
uint64_t __bake_c_atomic_load_u64(__bake_c_U64Array a,double i){return __atomic_load_n(atomic64(a,i),__ATOMIC_SEQ_CST);}
uint64_t __bake_c_atomic_store_u64(__bake_c_U64Array a,double i,uint64_t v){__atomic_store_n(atomic64(a,i),v,__ATOMIC_SEQ_CST);return v;}
double __bake_c_view_get_u16(__bake_c_View a,double i,bool le){return read_bits(a.buffer,i,2,le);}
double __bake_c_view_get_i16(__bake_c_View a,double i,bool le){return (int16_t)read_bits(a.buffer,i,2,le);}
double __bake_c_view_get_u32(__bake_c_View a,double i,bool le){return read_bits(a.buffer,i,4,le);}
double __bake_c_view_get_i32(__bake_c_View a,double i,bool le){return (int32_t)read_bits(a.buffer,i,4,le);}
uint64_t __bake_c_view_get_u64(__bake_c_View a,double i,bool le){return read_bits(a.buffer,i,8,le);}
uint64_t __bake_c_view_get_i64(__bake_c_View a,double i,bool le){return read_bits(a.buffer,i,8,le);}
void __bake_c_view_set_u16(__bake_c_View a,double i,double v,bool le){write_bits(a.buffer,i,2,le,u32(v));}
void __bake_c_view_set_u32(__bake_c_View a,double i,double v,bool le){write_bits(a.buffer,i,4,le,u32(v));}
void __bake_c_view_set_i32(__bake_c_View a,double i,double v,bool le){write_bits(a.buffer,i,4,le,u32(v));}
void __bake_c_view_set_u64(__bake_c_View a,double i,uint64_t v,bool le){write_bits(a.buffer,i,8,le,v);}
void __bake_c_view_set_i64(__bake_c_View a,double i,uint64_t v,bool le){write_bits(a.buffer,i,8,le,v);}
double __bake_c_view_get_f32(__bake_c_View a,double i,bool le){union {float f;uint32_t u;} v={.u=(uint32_t)read_bits(a.buffer,i,4,le)};return v.f;}
double __bake_c_view_get_f64(__bake_c_View a,double i,bool le){return value(read_bits(a.buffer,i,8,le));}
void __bake_c_view_set_f32(__bake_c_View a,double i,double v,bool le){union {float f;uint32_t u;} x={.f=(float)v};write_bits(a.buffer,i,4,le,x.u);}
void __bake_c_view_set_f64(__bake_c_View a,double i,double v,bool le){write_bits(a.buffer,i,8,le,bits(v));}
