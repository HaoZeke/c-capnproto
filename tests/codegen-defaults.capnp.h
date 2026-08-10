#ifndef CAPN_A68E823569A53E2E
#define CAPN_A68E823569A53E2E
/* AUTO GENERATED - DO NOT EDIT */
#include <capnp_c.h>

#if CAPN_VERSION != 1
#error "version mismatch between capnp_c.h and generated code"
#endif

#ifndef capnp_nowarn
# ifdef __GNUC__
#  define capnp_nowarn __extension__
# else
#  define capnp_nowarn
# endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

struct Rec;
struct Event;

typedef struct {capn_ptr p;} Rec_ptr;
typedef struct {capn_ptr p;} Event_ptr;

typedef struct {capn_ptr p;} Rec_list;
typedef struct {capn_ptr p;} Event_list;

enum Color {
	Color_red = 0,
	Color_green = 1,
	Color_blue = 2
};
#define ANSWER (42)
extern const int32_t answer;
#define FLAG (1)
extern const unsigned flag;
#define COUNT (7)
extern const uint16_t count;
#define BIG (((uint64_t) 0x1u << 32) | 0u)
extern const uint64_t big;
#define HUE (1u)
extern const enum Color hue;
extern const union capn_conv_f32 ratio;
#define ANSWER_CAMEL (99)
extern const int32_t answerCamel;

struct Rec {
	capn_data info;
	capn_ptr_list tags;
};

static const size_t Rec_word_count = 0;

static const size_t Rec_pointer_count = 2;

static const size_t Rec_struct_bytes_count = 16;


capn_data Rec_get_info(Rec_ptr p);

capn_ptr_list Rec_get_tags(Rec_ptr p);

void Rec_set_info(Rec_ptr p, capn_data info);

void Rec_set_tags(Rec_ptr p, capn_ptr_list tags);

struct Event {
	capn_ptr_list args;
};

static const size_t Event_word_count = 0;

static const size_t Event_pointer_count = 1;

static const size_t Event_struct_bytes_count = 8;


capn_ptr_list Event_get_args(Event_ptr p);

void Event_set_args(Event_ptr p, capn_ptr_list args);

Rec_ptr new_Rec(struct capn_segment*);
Event_ptr new_Event(struct capn_segment*);

Rec_list new_Rec_list(struct capn_segment*, int len);
Event_list new_Event_list(struct capn_segment*, int len);

void read_Rec(struct Rec*, Rec_ptr);
void read_Event(struct Event*, Event_ptr);

void write_Rec(const struct Rec*, Rec_ptr);
void write_Event(const struct Event*, Event_ptr);

void get_Rec(struct Rec*, Rec_list, int i);
void get_Event(struct Event*, Event_list, int i);

void set_Rec(const struct Rec*, Rec_list, int i);
void set_Event(const struct Event*, Event_list, int i);

#ifdef __cplusplus
}
#endif
#endif
