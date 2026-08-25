def loop -> howmany then 
    "Hello Guys!\n"
    $howmany = $howmany + 1
    if $howmany < 5 then 
        $howmany = loop($howmany)
    end 
    if $howmany equ 5 then 
        return 5
    end
end

loop(0) + loop(0) + 10
