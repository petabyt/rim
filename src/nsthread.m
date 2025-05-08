#import <Foundation/Foundation.h>

@interface CThreadWrapper : NSObject
@property (nonatomic, assign) void *arg;
@property (nonatomic, assign) void *(*func)(void *);
- (void)run;
@end

@implementation CThreadWrapper

- (void)run {
	if (self.func) {
		(void)self.func(self.arg);
	}
}

@end

void start_thread(void *(*func)(void *), void *arg) {
	CThreadWrapper *w = [CThreadWrapper new];
	w.func = func;
	w.arg = arg;

	NSThread *t = [[NSThread alloc] initWithTarget:w selector:@selector(run) object:nil];
	[t start];
}
