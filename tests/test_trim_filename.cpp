#include <trim_filename.h>

#include <gtest/gtest.h>

TEST(TestTrimFilename, fitsInRequestedSize)
{
    const std::string filename{R"(C:\iterated-dynamics\foo.par)"};
    
    const std::string result = trim_filename(filename, 80);

    EXPECT_EQ(filename, result);
}

TEST(TestTrimFilename, dropsIntermediateDirectories)
{
    const std::string filename{R"(C:\code\iterated-dynamics\build-default\install\pars\foo.par)"};
    
    const std::string result = trim_filename(filename, 44);

               //12345678901234567890123456789012345678901234
    EXPECT_GE(44U, result.size());
    EXPECT_EQ(R"(C:\code\...\install\pars\foo.par)", result);
}