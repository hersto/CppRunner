
g++ -O3 CppRunner.cpp -o /usr/bin/cpprunner
if [ $# != 0 ]
then
exit 1
fi

echo "Compiled successfull."
echo
echo
echo "In case you want to use VIM, follow this to activate syntax highlighting:"
echo
echo "  In   /usr/share/vim/current/scripts.vim   there is a if-elseif section for various interpreter names"
echo "  found in the shebang line (first line starting with '#!'). Add this to the elseif section and you'll"
echo "  have C++ syntax highlighting for your cpprunner scripts."
echo
echo " elseif s:name =~ 'cpprunner'"
echo "   set ft=cpp"
echo "     TBD: Would actually have to examine the second line if it has a //#! to see which compiler is actually"
echo "          called and in that case may set to another language."
echo



