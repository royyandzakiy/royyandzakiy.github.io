idf cmake components (jan)

https://github.com/royyandzakiy/idf-component-cmake

vefore ive stumbled upon the need to add local and external comppnents to idf projects. bcs idf uses its own cmake script for adding components, we need to fillow their soecific idiom and file structuring

the most batural way of doing it is just by assuming the main app is a single compnent that we just add all .cpp and .h files to. this will sometimes cause inflexibility when wanting to actuvate or deactivate different submodules or parts of the code. another native way to do it is by adding them outside the main, and inside the components. but for me this feels like a place for second class or added libraries that are not core. somi usually add any added git submodules or reused internal components inside this folder

in the next article i will talk about how to sperate the cmakelists of components inside the main folder