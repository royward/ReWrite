    int64_t result0;
    int64_t result1;
    bool resultb;
    int ret_code;

    printf("fact:");
    ret_code=rw_fact(exe,10,&result0);
    if(!ret_code && result0==3628800) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,result0); };

    printf("min_max:");
    ret_code=rw_min_max(exe,6,7,&result0,&result1);
    if(!ret_code && result0==6 && result1==7) { printf("success\n"); } else { printf("fail:%d %ld %ld\n",ret_code,result0,result1); };

    printf("max_min:");
    ret_code=rw_min_max(exe,6,7,&result0,&result1);
    if(!ret_code && result0==6 && result1==7) { printf("success\n"); } else { printf("fail:%d %ld %ld\n",ret_code,result0,result1); };

    printf("clamp:");
    ret_code=rw_clamp(exe,3,5,10,&result0);
    if(!ret_code && result0==5) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,result0); };

    printf("clamp:");
    ret_code=rw_clamp(exe,8,5,10,&result0);
    if(!ret_code && result0==8) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,result0); };

    printf("clamp:");
    ret_code=rw_clamp(exe,13,5,10,&result0);
    if(!ret_code && result0==10) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,result0); };

    printf("clamp:");
    ret_code=rw_clamp(exe,3,10,5,&result0);
    if(!ret_code && result0==5) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,result0); };

    printf("eval:");
    ret_code=rw_eval(exe,3,10,6,&result0);
    if(!ret_code && result0==4) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,result0); };

    printf("eval:");
    ret_code=rw_eval(exe,10,10,6,&result0);
    if(!ret_code) {
        printf("fail\n");
    } else {
        const char* function;
        uint32_t line;
        rw_instance_get_error(exe,&line,&function);
        if(ret_code==3 && strcmp(function,"eval")==0) {
            printf("success\n");
        } else {
            printf("fail:%d %s\n",ret_code,function);
        }
    };

    printf("equal:");
    ret_code=rw_equal(exe,6,7,&resultb);
    if(!ret_code && !resultb) { printf("success\n"); } else { printf("fail:%d %d\n",ret_code,(int)resultb); };

    printf("equal:");
    ret_code=rw_equal(exe,7,7,&resultb);
    if(!ret_code && resultb) { printf("success\n"); } else { printf("fail:%d %d\n",ret_code,(int)resultb); };

    printf("fact_tail:");
    ret_code=rw_fact_tail(exe,10,&result0);
    if(!ret_code && result0==3628800) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,result0); };

    printf("fact2:");
    ret_code=rw_fact2(exe,10,&result0);
    if(!ret_code && result0==3628800) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,result0); };

    printf("fib:");
    ret_code=rw_fib(exe,10,&result0);
    if(!ret_code && result0==89) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,result0); };

    printf("swap:");
    ret_code=rw_swap(exe,6,7,&result0,&result1);
    if(!ret_code && result0==7 && result1==6) { printf("success\n"); } else { printf("fail:%d %ld %ld\n",ret_code,result0,result1); };

    printf("multiret:");
    ret_code=rw_multiret(exe,10,6,&result0);
    if(!ret_code && result0==-3) { printf("success\n"); } else { printf("fail:%d %ld\n",ret_code,result0); };

    printf("neg:");
    ret_code=rw_neg(exe,true,&resultb);
    if(!ret_code && !resultb) { printf("success\n"); } else { printf("fail:%d %d\n",ret_code,(int)resultb); };

    printf("neg:");
    ret_code=rw_neg(exe,false,&resultb);
    if(!ret_code && resultb) { printf("success\n"); } else { printf("fail:%d %d\n",ret_code,(int)resultb); };
