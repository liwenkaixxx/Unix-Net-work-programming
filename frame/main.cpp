#include <stdio.h>
#include "frame.h"
#include "logger/logger.h"

//������
int main(int argc, char **argv)
{
    int ret = _main(argc, argv);
    LOG(LEVEL_I) << "main over. code:" << ret;
    return ret;
}
