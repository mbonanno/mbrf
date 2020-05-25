// remember to rebuild shaders manually if changing the values! (until I provide a decent shader building solution...)

#define UNIFORM_BUFFER_SLOT(n) 0 + n
#define MAX_UNIFORM_BUFFER_SLOTS 1
#define TEXTURE_SLOT(n) UNIFORM_BUFFER_SLOT(MAX_UNIFORM_BUFFER_SLOTS) + n
#define MAX_TEXTURE_SLOTS 16

#define MAX_NUM_BINDINGS (MAX_UNIFORM_BUFFER_SLOTS + MAX_TEXTURE_SLOTS)
