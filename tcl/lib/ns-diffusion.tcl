# All diffusion related procedures go here


Simulator instproc attach-diffapp { node diffapp } {
    set dr [$node create-diffusion-routing]
    $diffapp dr $dr
}


Node instproc create-diffusion-routing {} {
    $self instvar gradient_ diffAppAgent_
    
    # first we create diffusion agent
    # if it doesnot exist already
    # then we start the gradient filter
    if [info exists diffAppAgent_] {
	puts "diffAppAgent_ exists: $diffAppAgent_"
	return $diffAppAgent_
    }
    puts "creating new DiffAppAgent_"
    $self set diffAppAgent_ [new Agent/DiffusionApp]
    set da $diffAppAgent_
    set port [get-da-port $da $self]
    $da agent-id $port
    
    set gradient_ [new Application/GradientFilter $da] 
    #set gradient_ [new Application/GradientFilter]
    #$gradient_ dr $da
    #set dbug 10
    #$gradient_ debug $dbug
    
    return $da
}


proc get-da-port {da node} {
    #set nullagent [[Simulator instance] nullagent]
    #set port [$node alloc-port $nullagent]  
    # diffusion assumes diffusion-application agent
    # to be attached to non-zero port numbers
    # thus for now assigning port 254 to diffAppAgent
    set port 254
    $node attach $da $port
    return $port
}
