//
//  17_chapter.c
//  TCP&C-Demo
//
//  Created by sheng wang on 2019/11/20.
//  Copyright © 2019 feisu. All rights reserved.
//

#include "unp.c"

//调用 ”./a.out inet4 0“
int
main(int argc, char **argv)
{
    get_all_interface_info(argc, argv);
    exit(0);
}
