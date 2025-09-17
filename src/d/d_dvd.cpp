#include "d/d_dvd.h"

#include "m/m_dvd.h"

// clang-format off
#include "sized_string.h"
// clang-format on
namespace dDvd {

void create(s32 priority, EGG::Heap *archiveHeap, EGG::Heap *commandHeap, EGG::Heap *threadHeap) {
    mDvd::create(priority, archiveHeap, commandHeap, threadHeap);
}

loader_c::loader_c() {
    mpCommand = nullptr;
    mSize = -1;
    mpHeap = nullptr;
    mpBuffer = nullptr;
}

loader_c::~loader_c() {}

void *loader_c::request(const char *path, u8 mountDirection, EGG::Heap *heap) {
    if (mpBuffer != nullptr) {
        return mpBuffer;
    }
    if (mpCommand == 0) {
        mSize = -1;
        mpHeap = heap != nullptr ? heap : mDvd::getArchiveHeap();
        SizedString<128> buf;
        buf.sprintf("US%s", path);
        if (!mDvd::IsExistPath(buf)) {
            buf.sprintf("%s", path);
        }
        mpCommand = mDvd_toMainRam_normal_c::create(buf, mountDirection, heap);
    }
    if (mpCommand != nullptr && mpCommand->isDone()) {
        mpBuffer = mpCommand->mDataPtr;
        mSize = mpCommand->mAmountRead;
        mpCommand->mDataPtr = nullptr;
        mpCommand->do_delete();
        mpCommand = nullptr;
        return mpBuffer;
    } else {
        return nullptr;
    }
}

void loader_c::remove() {
    if (mpHeap != nullptr && mpBuffer != nullptr) {
        mpHeap->free(mpBuffer);
        mpBuffer = nullptr;
    }
}

} // namespace dDvd
