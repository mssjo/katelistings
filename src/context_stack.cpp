#include "language.hpp"

void language::context_stack::switch_context(const language::context_switch& con_sw, 
                                             const std::smatch& new_match)
{
    
//     std::cout << "Switching contexts\n";
    
    //Pop as ordered, but do not pot default context
    for(size_t i = 0; i < con_sw.pops; i++){
        if(stack.size() == 1)
            break;
        
        stack.pop();
    }
    
    //Push new context if ordered
    if(con_sw.target)
        stack.push( std::make_pair( con_sw.target, new_match ) );   
    
//     if(con_sw.pops > 0 || con_sw.target != nullptr)
//         std::cout << "\tSwitched to context \"" << curr_context().get_name() << "\"\n";
}
