//===-- Unittests for readlink --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "src/__support/CPP/string_view.h"
#include "src/errno/libc_errno.h"
#include "src/unistd/readlink.h"
#include "src/unistd/symlink.h"
#include "src/unistd/unlink.h"
#include "test/UnitTest/ErrnoSetterMatcher.h"
#include "test/UnitTest/Test.h"

namespace cpp = LIBC_NAMESPACE::cpp;

TEST(LlvmLibcReadlinkTest, CreateAndUnlink) {
  using LIBC_NAMESPACE::testing::ErrnoSetterMatcher::Succeeds;
  constexpr const char LINK_VAL[] = "readlink_test_value";
  constexpr const char LINK[] = "testdata/readlink.test.link";
  libc_errno = 0;

  // The test strategy is as follows:
  //   1. Create a symlink with value LINK_VAL.
  //   2. Read the symlink with readlink. The link value read should be LINK_VAL
  //   3. Cleanup the symlink created in step #1.
  ASSERT_THAT(LIBC_NAMESPACE::symlink(LINK_VAL, LINK), Succeeds(0));

  char buf[sizeof(LINK_VAL)];
  ssize_t len = LIBC_NAMESPACE::readlink(LINK, buf, sizeof(buf));
  ASSERT_ERRNO_SUCCESS();
  ASSERT_EQ(cpp::string_view(buf, len), cpp::string_view(LINK_VAL));

  ASSERT_THAT(LIBC_NAMESPACE::unlink(LINK), Succeeds(0));
}

TEST(LlvmLibcReadlinkTest, ReadlinkInNonExistentPath) {
  using LIBC_NAMESPACE::testing::ErrnoSetterMatcher::Fails;
  char buf[8];
  ASSERT_THAT(LIBC_NAMESPACE::readlink("non-existent-link", buf, sizeof(buf)),
              Fails(ENOENT));
}
