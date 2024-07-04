/*
 * gui task - defines public interface for gui task to be started
 */

 // how much memory for the guiTask stack. 
#define kGuiStackSize (4096*2)

void guiTask(void *pvParameter);
