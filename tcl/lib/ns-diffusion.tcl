# All diffusion related procedures go here


Simulator instproc attach-diffapp { node diffapp } {
    $diffapp dr [$node get-dr]
}


Node instproc get-dr {} {
    $self instvar diffAppAgent_
    if [info exists diffAppAgent_] {
	return $diffAppAgent_
    } else {
	puts "Error: No DiffusionApp agent created for this node!\n" 
	exit 1
    }
}


Node instproc create-diffusionApp-agent {} {
    $self instvar gradient_ diffAppAgent_
    
    # first we create diffusion agent
    # if it doesnot exist already
    # then we start the gradient filter
    if [info exists diffAppAgent_] {
	puts "diffAppAgent_ exists: $diffAppAgent_"
	return $diffAppAgent_
    }
    #puts "creating new DiffAppAgent_"
    $self set diffAppAgent_ [new Agent/DiffusionApp]
    set da $diffAppAgent_
    set port [get-da-port $da $self]
    $da agent-id $port
    
    set gradient_ [new Application/GradientFilter $da] 
    #$gradient_ debug 10
    
    return $da
}


proc get-da-port {da node} {

    # diffusion assumes diffusion-application agent
    # to be attached to non-zero port numbers
    # thus for assigning port 254 to diffAppAgent
    set port [Node set DIFFUSION_APP_PORT]
    $node attach $da $port
    return $port
}
