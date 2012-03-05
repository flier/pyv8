import PyV8

env_variables = {
    'data' : [4,9,2,13,5]
}
context = PyV8.JSContext(env_variables)
context.enter()

script = """
data.constructor
//data.max();
"""

print context.eval(script)
context.leave()
