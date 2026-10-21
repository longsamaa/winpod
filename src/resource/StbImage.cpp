// Phần cài đặt của stb_image (header-only): chỉ define ở đúng file này.
// Chỉ bật PNG cho gọn; thêm STBI_ONLY_xxx nếu cần định dạng khác.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_WINDOWS_UTF8

#ifdef _MSC_VER
#pragma warning(push, 0) // code bên thứ ba, không áp /W4 của project
#endif
#include <stb_image.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
