idf cmake components 2 (jan)

https://github.com/royyandzakiy/idf-component-cmake

here will highlight how to sperate cmakelists of compknents inisde the main. u found there to be 2 ways to do this. first way is just by using a raw cmake add library call. for those familiar, this uses the natuve cmake functions ( ot the custom idf ones). we will proceed by calling add subdirectory and link library to add it. 

another way that needs a little hack but enables us to just use the native idf func calls is by  first adding the added comp as an extra component dir. here we will understand that idf has only 2 prederined compjnent filders, then main, actung as one hugh compjnent, and the root compnent. so if we want to define other parts as compnents, we need to declare that folder (even tho its inside the main fold). after thatw e will just need to add it as a required compnent for main. this way its clean and easy to actuvate or deactivate. i personally like this way more, to hide away details of .cpp and .h just within their own cmakelist